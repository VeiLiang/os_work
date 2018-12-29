/*********************************************************************
File        : MMC_SD_Drive.c
Purpose     : Device Driver for MMC SD CARD
---------------------------END-OF-HEADER------------------------------
*/
//#include "hardware.h"
#include "sdmmc.h"
#include "FS.h"
#include "fs_card.h"
#include "fs_port.h"
#include "printk.h"
#include "xmprintf.h"

 #define NUM_UNITS          1

static int _NumUnits;

#if SD_DEBUG
	#define DEBUG printk
#else
	#define DEBUG 
#endif

#ifdef __cplusplus
extern "C" {
#endif

  void* OS_malloc(unsigned int);
  void  OS_free  (void* pMemBlock);
  void* OS_realloc  (void* pMemBlock, unsigned NewSize);
  void* OS_calloc (unsigned int bytes,unsigned int size);
#ifdef __cplusplus
}
#endif

DWORD XM_GetTickCount (void);
static U32 total_read_sector_count = 0;
static U32 total_read_times = 0;
static U32 total_write_sector_count = 0;
static U32 total_write_times = 0;

static U32 start_ticket;
static U32 read_sector_count = 0;
static U32 read_times = 0;
static U32 write_sector_count = 0;
static U32 write_times = 0;


static void stat(void)
{
	if( read_sector_count == 0 && write_sector_count == 0)
	{
		start_ticket = XM_GetTickCount();
	}
	else 
	{
		U32 tickets = XM_GetTickCount() - start_ticket;
		if(tickets >= 10000)	// 10秒
		{
			XM_printf ("read_sector_count = %d, read_times = %d, write_sector_count = %d, write_times=%d, ticket=%d\n", 
					  read_sector_count, read_times, write_sector_count, write_times, tickets); 
			XM_printf ("read_sector_total = %d, read_total = %d, write_sector_total = %d, write_total=%d\n", 
					  total_read_sector_count, total_read_times, total_write_sector_count, total_write_times); 
			
			// 重新统计
			read_sector_count = 0;
			write_sector_count = 0;
			read_times = 0;
			write_times = 0;
			start_ticket = XM_GetTickCount();
		}
	}
}


volatile FS_CARD SDCard={
	0, //Present = 0,
	1, //Changed = 1,
	0  //SecNum  = 0
};


/*********************************************************************
*
*       Static code
*
**********************************************************************
*/
/*********************************************************************
*
*       _GetDriverName
*/
static const char * _GetDriverName(U8 Unit) {
	FS_USE_PARA(Unit);
	return "mmc";
}
/*********************************************************************
*
*       _AddDevice
*/
static int _AddDevice(void) {
	if (_NumUnits >= NUM_UNITS) {
		return -1;
  }
  return _NumUnits++;
}


/*********************************************************************
*
*       _Read
*
*  Description:
*    FS driver function. Read a sectors from the disk.
*
*  Parameters:
*    Unit        - Unit number.
*    SectorNo    - Sector to be read from the device.
*    pBuffer     - Pointer to buffer for storing the data.
*    NumSectors  - number of sectors to be read
*
*  Return value:
*    ==0         - Sector has been read and copied to pBuffer.
*    <0          - An error has occured.
*/
static int _Read(U8 Unit, U32 SectorNo, void *pBuffer, U32 NumSectors)
{
	int ret;
	if (Unit >= NUM_UNITS)
	{
		return -1;  /* No valid unit number */
	}

	stat ();
	read_sector_count += NumSectors;
	read_times ++;
	total_read_sector_count += NumSectors;
	total_read_times ++;


	ret = SD_ReadSector(sdmmc_chip0, Unit, SectorNo, pBuffer, NumSectors);
	
	
	if (ret != 0)
	{
		printf ("SD read sector error\n");
		ret = -1;
	}
	
	return ret;
}

#define	ENABLE_REPEAT_LARGE_BUFFER
/*********************************************************************
*
*       _Write
*
*  Description:
*    Driver callback function.
*    Writes one or more logical sectors to storage device.
*
*  Parameters:
*    Unit        - Unit number.
*    SectorNo    - Sector number to be written to the device.
*    pBuffer     - Pointer to buffer conating the data to write
*    NumSectors  - Number of sectors to store.
*    RepeatSame  - Repeat the same data to sectors.
*
*  Return value:
*      0                       - Data successfully written.
*    !=0                       - An error has occurred.
*
*/
static int _Write(U8 Unit, U32 SectorNo, const void *pBuffer, U32 NumSectors, U8 RepeatSame)
{
	int ret;
	int repeat_count;
	
	//printf ("_Write SectorNo=%d, NumSectors=%d, RepeatSame=%d\n", SectorNo, NumSectors, RepeatSame);
	//if(NumSectors == 216)
	//{
	//	printf ("strange count %d\n", NumSectors);
	//}
	
	if (Unit >= NUM_UNITS)
	{
		return -1;  /* No valid unit number */
	}
	
	stat ();
	write_sector_count += NumSectors;
	write_times += 1;
	total_write_sector_count += NumSectors;
	total_write_times += 1;
	
	
	if(RepeatSame)
	{		
		// 在格式化操作时设置RepeatSame = 1
#ifdef ENABLE_REPEAT_LARGE_BUFFER
		// 分配大的缓存加速写入过程(优化SD卡格式化速度)
		char *large_buffer = NULL;
		int large_count = 512;
		if(large_count > (int)NumSectors)
			large_count = NumSectors;
		while(!large_buffer)
		{
			large_buffer = OS_malloc (large_count * 512);
			if(large_buffer)
				break;
			large_count >>= 1;
		}
		if(large_buffer)
		{
			int i = 0;
			while(i < large_count)
			{
				memcpy (large_buffer + i * 512, pBuffer, 512);
				i ++;
			}
			
			while(NumSectors > 0)
			{
				U32 ToWrite = NumSectors;
				if((int)ToWrite > large_count)
					ToWrite = large_count;
				
				repeat_count = 3;
				ret = -1;
				while(repeat_count > 0)
				{
					ret = SD_WriteSector(sdmmc_chip0, Unit, SectorNo, (UINT8*)large_buffer, ToWrite, 0);
					if(ret == 0)
						break;
					repeat_count --;
				}
				if(ret != 0)	// error, 
					break;
				
				SectorNo += ToWrite;
				NumSectors -= ToWrite;
			}
			
			OS_free (large_buffer);
		}
		else
#endif
		{
			// 大的缓存分配失败
			while(NumSectors > 0)
			{
				// 失败重复3次
				repeat_count = 3;
				ret = -1;
				while(repeat_count > 0)
				{
					ret = SD_WriteSector(sdmmc_chip0, Unit, SectorNo, (UINT8*)pBuffer, 1, 0);
					if(ret == 0)
						break;
					repeat_count --;
				}
				if(ret != 0)	// error, 
					break;
				SectorNo ++;
				NumSectors --;
			}
		}
	}
	else
	{
		static volatile int reenter = 0;
		if(reenter)
		{
			printf ("reentry\n");
		}
		else
		{
			reenter ++;
		}
		while(NumSectors > 0)
		{
			U32 ToWrite = NumSectors;
			if(ToWrite > (SDMMC_MAX_BLOCK_SIZE/512))
				ToWrite = (SDMMC_MAX_BLOCK_SIZE/512);
			ret = -1;
			// 失败重复3次
			repeat_count = 3;
			while(repeat_count > 0)
			{
				if(repeat_count != 3)
				{
					printf ("re-execute SD_WriteSector, SectorNo=%d,  ToWrite=%d\n", SectorNo, ToWrite);
				}
	 			ret = SD_WriteSector(sdmmc_chip0, Unit, SectorNo, (UINT8*)pBuffer, ToWrite, 0);
				if(ret == 0)
					break;
				repeat_count --;
			}
			if(ret != 0)	//error
			{
				printf ("SD write sector error\n");
				break;
			}
			SectorNo += ToWrite;
			pBuffer = (char *)pBuffer + ToWrite * 512;
			NumSectors -= ToWrite;
		}
		
		reenter --;
		
	}
	if (ret != 0)
	{
		ret = -1;
	}
	return ret;
}

static int _WriteMultBuffer (U8 Unit, U32 SectorNo, U32 NumSectors, U32 NumBuffers, const U8 **pSectorBuffer, U32 *pSectorCount)
{
	int repeat_count;
	int ret;
	if (Unit >= NUM_UNITS)
	{
		return -1;  /* No valid unit number */
	}
	
	stat ();
	write_sector_count += NumSectors;
	write_times += NumBuffers;
	total_write_sector_count += NumSectors;
	total_write_times += NumBuffers;
	
	repeat_count = 3;
	ret = -1;
	while(repeat_count > 0)
	{
		ret = SD_WriteMultiBuffer (sdmmc_chip0, SectorNo, NumSectors, NumBuffers, (const UINT8 **)pSectorBuffer, pSectorCount);
		if(ret == 0)
			break;
		repeat_count --;
	}
	if(ret != 0)	// error
	{
		printf ("SD write multi-buffer error\n");
		ret = -1;
	}
	return ret;
}

static int _ReadMultBuffer (U8 Unit, U32 SectorNo, U32 NumSectors, U32 NumBuffers,  U8 **pSectorBuffer, U32 *pSectorCount)
{
	int repeat_count;
	int ret;
	if (Unit >= NUM_UNITS)
	{
		return -1;  /* No valid unit number */
	}
	
	stat ();
	read_sector_count += NumSectors;
	read_times += NumBuffers;
	total_read_sector_count += NumSectors;
	total_read_times += NumBuffers;

	repeat_count = 3;
	ret = -1;
	while(repeat_count > 0)
	{
		ret = SD_ReadMultiBuffer (sdmmc_chip0, SectorNo, NumSectors, NumBuffers, (UINT8 **)pSectorBuffer, pSectorCount);
		if(ret == 0)
			break;
		repeat_count --;
	}
	if(ret != 0)	// error
	{
		printf ("SD read multi-buffer error\n");
		ret = -1;
	}
	return ret;
}

/*********************************************************************
*
*       _IoCtl
*
*  Description:
*    FS driver function. Execute device command.
*
*  Parameters:
*    Unit        - Unit number.
*    Cmd         - Command to be executed.
*    Aux         - Parameter depending on command.
*    pBuffer     - Pointer to a buffer used for the command.
*
*  Return value:
*    Command specific. In general a negative value means an error.
*/
static int _IoCtl(U8 Unit, I32 Cmd, I32 Aux, void *pBuffer) 
{
	FS_DEV_INFO * pDevInfo;
	if (Unit >= NUM_UNITS)
	{
		return -1;  /* No valid unit number */
	}
 
	
	switch (Cmd) 
	{
		case FS_CMD_UNMOUNT:
			break;

		case FS_CMD_GET_DEVINFO: /* Get general device information */
			pDevInfo = (FS_DEV_INFO *)pBuffer;
			pDevInfo->BytesPerSector = 512;
			pDevInfo->NumSectors     = SDCard.SecNum;
			break;
		
		case FS_CMD_UNMOUNT_FORCED:
			break;

		case FS_CMD_SYNC:
			//
			// (Optional)
			// Sync/flush any pending operations 
			//
	
			// ToDo: Call the function
			break;
			
		default:
			return -1;
	}

	return 0;
}

/*********************************************************************
*
*       _MMC_InitMedium
*
*  Description:
*    Initialize the card.
*
*  Parameters:
*    Unit        - Unit number.
*
*  Return value:
*    == 0                       - Device okay and ready for operation.
*    <  0                       - An error has occured.
*/
static int _InitMedium(U8 Unit){
	if (Unit >= NUM_UNITS)
	{
		return -1;  /* No valid unit number */
	}
  return 0;
}
/*********************************************************************
*
*       _GetStatus
*
*  Description:
*    FS driver function. Get status of the disk.
*
*  Parameters:
*    Unit    - Device number.
*
*  Return value:
*    FS_MEDIA_STATE_UNKNOWN    - Media state is unknown
*    FS_MEDIA_NOT_PRESENT      - Media is not present
*    FS_MEDIA_IS_PRESENT       - Media is present
*/
static int _GetStatus(U8 Unit)
{
	//DEBUG ("_FS_SD_DevStatus0\n");

	if(!SDCard.Present)   //card not in socket
	{ 
		//DEBUG("card is not in the socket\n");
		return FS_MEDIA_NOT_PRESENT;
	}
	if (Unit >= NUM_UNITS)
	{
		return FS_MEDIA_NOT_PRESENT;  /* No valid unit number */
	}
	/* card changed */
	if(SDCard.Changed)
	{
		SDCard.Changed = 0; //reset the media changed flag

		return FS_MEDIA_IS_PRESENT;
	}
  //return FS_MEDIA_STATE_UNKNOWN;
	return FS_MEDIA_IS_PRESENT;
}
/*********************************************************************
*
*       _GetNumUnits
*/
static int _GetNumUnits(void) {
	return _NumUnits;
}
/*********************************************************************
*
*       Public data
*
**********************************************************************
*/
const FS_DEVICE_TYPE FS_SDMMC_Driver = {
  _GetDriverName,
  _AddDevice,
  _Read,
  _Write,
  _IoCtl,
  _InitMedium,
  _GetStatus,
  _GetNumUnits,
  _WriteMultBuffer,
  _ReadMultBuffer
};
