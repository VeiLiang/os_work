/*********************************************************************
File        : MMC_SD_Drive.c
Purpose     : Device Driver for MMC SD CARD
---------------------------END-OF-HEADER------------------------------
*/
#include "hardware.h"
#include "sdmmc.h"
#include "FS.h"
#include "fs_card.h"
#include "fs_port.h"
#include "printk.h"
#include "xm_core.h"

#ifdef FS_SDMMC_MAXUNITS
  #define NUM_UNITS          FS_SDMMC_MAXUNITS
#else
  #define NUM_UNITS          1
#endif

	// 20170511
	// SD卡读出时会出现TIMEOUT或CRC等错误, 此时重新读取1~3次, 一般会读出正确的内容.	
#define	FAILED_REPEAT_COUNT		4

static int _NumUnits;

#if SD_DEBUG
	#define DEBUG printk
#else
	#define DEBUG(...) 
#endif



DWORD XM_GetTickCount (void);


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

/*
static void fast_int32_512_copy (void *d, void *s, int count)
{
	// R3, R4, R5, R6, R7, R8, R9, R12
	asm ("push {r4, r5, r6, r7, r8, r9}");
	asm ("loop:");
	asm ("LDM r1!, {r3-r9, r12}");
	asm ("SUBS	r2, r2, #32");
	asm ("STM r0!, {r3-r9, r12}");
	asm ("BCS	loop");
	asm ("pop {r4, r5, r6, r7, r8, r9}");
	asm ("BX 	lr");
}*/

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
static int c_Read(U8 Unit, U32 SectorNo, void *pBuffer, U32 NumSectors)
{
	int ret;
	int loop;
	if (Unit >= NUM_UNITS)
	{
		return -1;  /* No valid unit number */
	}
	
	// 20170511
	// SD卡读出时会出现TIMEOUT或CRC等错误, 此时重新读取1~3次, 一般会读出正确的内容.	
	loop = FAILED_REPEAT_COUNT;
	while (loop > 0)
	{
		ret = SD_ReadSector(sdmmc_chip0, Unit, SectorNo, pBuffer, NumSectors);
		if(ret == 0)
			break;
		loop --;
	}
	if (ret != 0)
	{
	#ifdef DEBUG_MESSAGE
		printf ("SD read sector error\n");
	#endif
		ret = -1;
	}
	
	return ret;
}

static int _Read(U8 Unit, U32 SectorNo, void *pBuffer, U32 NumSectors)
{
	unsigned char old_priority;
	int ret;
	OS_TASK *pCurrentTask = OS_GetpCurrentTask();
	old_priority = OS_GetPriority (pCurrentTask);
	OS_SetPriority (pCurrentTask, 254);
	ret = c_Read (Unit, SectorNo, pBuffer, NumSectors);
	OS_SetPriority (pCurrentTask, old_priority);
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
static int c_Write(U8 Unit, U32 SectorNo, const void *pBuffer, U32 NumSectors, U8 RepeatSame)
{
	int ret;
	int repeat_count;
	
	//printf ("_Write SectorNo=%d, NumSectors=%d, RepeatSame=%d\n", SectorNo, NumSectors, RepeatSame);	
	if (Unit >= NUM_UNITS)
	{
		return -1;  /* No valid unit number */
	}
	
	
	if(RepeatSame)
	{		
		// 在格式化操作时设置RepeatSame = 1
#ifdef ENABLE_REPEAT_LARGE_BUFFER
		// 分配大的缓存加速写入过程(优化SD卡格式化速度)
		char *large_buffer = NULL;
		int large_count = 512;
		if(large_count > NumSectors)
			large_count = NumSectors;
		while(!large_buffer)
		{
			large_buffer = kernel_malloc (large_count * 512);
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
				if(ToWrite > large_count)
					ToWrite = large_count;
				
				repeat_count = FAILED_REPEAT_COUNT;
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
			
			kernel_free (large_buffer);
		}
		else
#endif
		{
			// 大的缓存分配失败
			while(NumSectors > 0)
			{
				// 失败重复3次
				repeat_count = FAILED_REPEAT_COUNT;
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
		while(NumSectors > 0)
		{
			U32 ToWrite = NumSectors;
			if(ToWrite > (SDMMC_MAX_BLOCK_SIZE/512))
				ToWrite = (SDMMC_MAX_BLOCK_SIZE/512);
			ret = -1;
			// 失败重复3次
			repeat_count = FAILED_REPEAT_COUNT;
			while(repeat_count > 0)
			{
				if(repeat_count != 3)
				{
#ifdef DEBUG_MESSAGE
					printf ("re-execute SD_WriteSector, SectorNo=%d,  ToWrite=%d\n", SectorNo, ToWrite);
#endif
				}
	 			ret = SD_WriteSector(sdmmc_chip0, Unit, SectorNo, (UINT8*)pBuffer, ToWrite, 0);
				if(ret == 0)
					break;
				repeat_count --;
			}
			if(ret != 0)	//error
			{
#ifdef DEBUG_MESSAGE
				printf ("SD write sector error\n");
#endif
				break;
			}
			SectorNo += ToWrite;
			pBuffer = (char *)pBuffer + ToWrite * 512;
			NumSectors -= ToWrite;
		}		
	}
	if (ret != 0)
	{
		ret = -1;
	}
	return ret;
}

static int _Write(U8 Unit, U32 SectorNo, const void *pBuffer, U32 NumSectors, U8 RepeatSame)
{
	unsigned char old_priority;
	int ret;
	OS_TASK *pCurrentTask = OS_GetpCurrentTask();
	old_priority = OS_GetPriority (pCurrentTask);
	OS_SetPriority (pCurrentTask, 254);
	ret = c_Write (Unit, SectorNo, pBuffer, NumSectors, RepeatSame);
	OS_SetPriority (pCurrentTask, old_priority);
	return ret;	
}

static int c_WriteMultBuffer (U8 Unit, U32 SectorNo, U32 NumSectors, U32 NumBuffers, const U8 **pSectorBuffer, U32 *pSectorCount)
{
	int repeat_count;
	int ret;
	if (Unit >= NUM_UNITS)
	{
		return -1;  /* No valid unit number */
	}
	
	repeat_count = FAILED_REPEAT_COUNT;
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
#ifdef DEBUG_MESSAGE
		printf ("SD write multi-buffer error\n");
#endif
		ret = -1;
	}
	return ret;
}

static int _WriteMultBuffer (U8 Unit, U32 SectorNo, U32 NumSectors, U32 NumBuffers, const U8 **pSectorBuffer, U32 *pSectorCount)
{
	unsigned char old_priority;
	int ret;
	OS_TASK *pCurrentTask = OS_GetpCurrentTask();
	old_priority = OS_GetPriority (pCurrentTask);
	OS_SetPriority (pCurrentTask, 254);
	ret = c_WriteMultBuffer (Unit, SectorNo, NumSectors, NumBuffers, pSectorBuffer, pSectorCount);
	OS_SetPriority (pCurrentTask, old_priority);
	return ret;		
}

static int c_ReadMultBuffer (U8 Unit, U32 SectorNo, U32 NumSectors, U32 NumBuffers,  U8 **pSectorBuffer, U32 *pSectorCount)
{
	int repeat_count;
	int ret;
	if (Unit >= NUM_UNITS)
	{
		return -1;  /* No valid unit number */
	}
		
	repeat_count = FAILED_REPEAT_COUNT;
	ret = -1;
	while(repeat_count > 0)
	{
		ret = SD_ReadMultiBuffer (sdmmc_chip0, SectorNo, NumSectors, NumBuffers, (const UINT8 **)pSectorBuffer, pSectorCount);
		if(ret == 0)
			break;
		repeat_count --;
	}
	if(ret != 0)	// error
	{
#ifdef DEBUG_MESSAGE
		printf ("SD read multi-buffer error\n");
#endif
		ret = -1;
	}
	return ret;
}

static int _ReadMultBuffer (U8 Unit, U32 SectorNo, U32 NumSectors, U32 NumBuffers,  U8 **pSectorBuffer, U32 *pSectorCount)
{
	unsigned char old_priority;
	int ret;
	OS_TASK *pCurrentTask = OS_GetpCurrentTask();
	old_priority = OS_GetPriority (pCurrentTask);
	OS_SetPriority (pCurrentTask, 254);
	ret = c_ReadMultBuffer (Unit, SectorNo, NumSectors, NumBuffers, pSectorBuffer, pSectorCount);
	OS_SetPriority (pCurrentTask, old_priority);
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
