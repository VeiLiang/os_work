/*
**********************************************************************
*                          Micrium, Inc.
*                      949 Crestview Circle
*                     Weston,  FL 33327-1848
*
*                            uC/FS
*
*             (c) Copyright 2001 - 2006, Micrium, Inc.
*                      All rights reserved.
*
***********************************************************************

----------------------------------------------------------------------
----------------------------------------------------------------------
File        : FS_LogBlock.c
Purpose     : Logical Block Layer
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "FS_ConfDefaults.h"        /* FS Configuration */
#include "FS_Int.h"

/*********************************************************************
*
*       Macros
*
**********************************************************************
*/

#if FS_DEBUG_LEVEL >= FS_DEBUG_LEVEL_CHECK_PARA
  #define INC_READ_BURST_CNT()                      _Stat.ReadBurstCnt++
  #define INC_READ_BURST_SECTOR_CNT(NumSectors)     _Stat.ReadBurstSectorCnt += NumSectors
  #define INC_READ_SECTOR_CNT()                     _Stat.ReadSectorCnt++
  #define INC_WRITE_BURST_CNT()                     _Stat.WriteBurstCnt++
  #define INC_WRITE_BURST_SECTOR_CNT(NumSectors)    _Stat.WriteBurstSectorCnt += NumSectors
  #define INC_WRITE_MULTIPLE_CNT()                  _Stat.WriteMultipleCnt++
  #define INC_WRITE_MULTIPLE_SECTOR_CNT(NumSectors) _Stat.WriteMultipleSectorCnt += NumSectors
  #define INC_WRITE_SECTOR_CNT()                    _Stat.WriteSectorCnt++
#else
  #define INC_READ_BURST_CNT()
  #define INC_READ_BURST_SECTOR_CNT(NumSectors)
  #define INC_READ_SECTOR_CNT()
  #define INC_WRITE_BURST_CNT()
  #define INC_WRITE_BURST_SECTOR_CNT(NumSectors)
  #define INC_WRITE_MULTIPLE_CNT()
  #define INC_WRITE_MULTIPLE_SECTOR_CNT(NumSectors)
  #define INC_WRITE_SECTOR_CNT()
#endif

/*********************************************************************
*
*       Code & data for debug builds
*
**********************************************************************
*/
#if FS_DEBUG_LEVEL >= FS_DEBUG_LEVEL_LOG_ALL

typedef struct {
  int Type;
  const char *s;
} TYPE_DESC;

#define TYPE2NAME(Type)  { Type,          #Type }

static const TYPE_DESC _aDesc[] = {
  { FS_SECTOR_TYPE_DATA, "DATA" },
  { FS_SECTOR_TYPE_MAN,  "MAN " },
  { FS_SECTOR_TYPE_DIR,  "DIR " },
};

/*********************************************************************
*
*       _Type2Name
*
*/
// static   -- Code is static, but not declared as such in order to avoid compiler warnings if this function is not referenced (lower debug levels)
const char * _Type2Name(int Type);    // Avoid "No prototype" warning
const char * _Type2Name(int Type) {
  unsigned i;
  for (i = 0; i < COUNTOF(_aDesc); i++) {
    if (_aDesc[i].Type == Type) {
      return _aDesc[i].s;
    }
  }
  return "Unknown Type";
}

#endif

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

#if FS_DEBUG_LEVEL >= FS_DEBUG_LEVEL_CHECK_PARA
static FS_STAT _Stat;
#endif



/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

#if FS_SUPPORT_BUSY_LED
  #define CLR_BUSY_LED(pDevice) _ClrBusyLED(pDevice)
  #define SET_BUSY_LED(pDevice) _SetBusyLED(pDevice)


/*********************************************************************
*
*       _ClrBusyLED
*
*  Function description
*    Calls the user supplied callback (hook) to switch off the BUSY-LED.
*/
static void _ClrBusyLED(FS_DEVICE * pDevice) {
  if (pDevice->Data.pfSetBusyLED) {
    pDevice->Data.pfSetBusyLED(0);
  }
}

/*********************************************************************
*
*       _SetBusyLED
*
*  Function description
*    Calls the user supplied callback (hook) to switch on the BUSY-LED.
*/
static void _SetBusyLED(FS_DEVICE * pDevice) {
  if (pDevice->Data.pfSetBusyLED) {
    pDevice->Data.pfSetBusyLED(1);
  }

}

#else
  #define CLR_BUSY_LED(pDevice)
  #define SET_BUSY_LED(pDevice)
#endif

/*********************************************************************
*
*       _Read
*
*  Function description
*    Static helper function for Write either to device or thru journal.
*
*  Return value
*    ==0         - All sectors have been read sucessfully.
*    <0          - An error has occured.
*
*/
static int _Read(FS_DEVICE * pDevice, U32 SectorNo, void * pBuffer, U32 NumSectors, U8 Type) {
  int r;
  const FS_DEVICE_TYPE * pDeviceType;
  pDeviceType = pDevice->pType;
#if FS_SUPPORT_JOURNAL
  if (pDevice->Data.JournalIsActive) {
    r = FS_JOURNAL_Read(pDevice, SectorNo, pBuffer, NumSectors);
  } else
#endif
  {
    r = (pDeviceType->pfRead)(pDevice->Data.Unit, SectorNo, pBuffer, NumSectors);
  }
#if FS_SUPPORT_CACHE
  if (pDevice->Data.pCacheAPI) {
    if (r == 0) {                   // Read from device was successfully
      U16 SectorSize;

      SectorSize = FS_GetSectorSize(pDevice);
      do {
        if (pDevice->Data.pCacheAPI->pfUpdateCache(pDevice, SectorNo, pBuffer, Type)) {
          FS_DEBUG_WARN("Could not update sector in cache");
        }
        SectorNo++;
        pBuffer = (void *)((U8 *)pBuffer + SectorSize);
      } while (--NumSectors);
    }
  }
#endif
  return r;
}

/*********************************************************************
*
*       _Write
*
*  Function description
*    Static helper function for Write either to device or thru journal.
*
*  Return value
*    ==0           - All sectors have been written successfully.
*    < 0           - An error has occured.
*
*/
static int _Write(const FS_DEVICE * pDevice, U32 SectorNo, const void * pBuffer, U32 NumSectors, U8 RepeatSame) {
  int r;
  const FS_DEVICE_TYPE * pDeviceType;
  pDeviceType = pDevice->pType;
#if FS_SUPPORT_JOURNAL
  if (pDevice->Data.JournalIsActive) {
    r = FS_JOURNAL_Write(pDevice, SectorNo, pBuffer, NumSectors, RepeatSame);
  } else
#endif
  {
    r = (pDeviceType->pfWrite)(pDevice->Data.Unit, SectorNo, pBuffer, NumSectors, RepeatSame);
  }
  return r;
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*
*        They should not be called by user application.
*/

/*********************************************************************
*
*       FS_LB_GetStatus
*
*  Function description
*    FS internal function. Get status of a device.
*
*  Parameters
*    pDevice     - Pointer to a device driver structure.
*
*  Return value
*    FS_MEDIA_STATE_UNKNOWN  if the state of the media is unknown.
*    FS_MEDIA_NOT_PRESENT    if media is not present.
*    FS_MEDIA_IS_PRESENT     if media is     present.
*
*/
int FS_LB_GetStatus(const FS_DEVICE * pDevice) {
  int x;
  char * DeviceType;
  const FS_DEVICE_TYPE * pDeviceType;
  pDeviceType = pDevice->pType;

  DeviceType = (char*)(pDeviceType->pfGetName)(pDevice->Data.Unit);

//  printk("FS_LB_GetName = %s \r\n", DeviceType);

//  printk("FS_LB_GetStatus pDevice->Data.Unit = %d \r\n", pDevice->Data.Unit);
  
  x = (pDeviceType->pfGetStatus)(pDevice->Data.Unit);
  return x;
}

/*********************************************************************
*
*       FS_LB_InitMedium
*
*  Function description
*    This function calls the initialize routine of the driver, if one exists.
*    If there if no initialization routine available, we assume the driver is
*    handling this automatically.
*
*  Parameters
*    pDevice     - Pointer to a device driver structure.
*
*  Return value
*    1           - Device/medium has been initialized.
*    0           - Error, device/medium could not be initialized.
*/
int FS_LB_InitMedium(FS_DEVICE * pDevice) {
  int IsInited = 0;
  const FS_DEVICE_TYPE * pDeviceType;

  pDeviceType = pDevice->pType;
  if (pDeviceType->pfInitMedium) {
    if ((pDeviceType->pfInitMedium)(pDevice->Data.Unit) == 0) {
      IsInited = 1;
    }
  } else {
    IsInited = 1;
  }
  pDevice->Data.IsInited = IsInited;
  return IsInited;
}

/*********************************************************************
*
*       FS_LB_InitMediumIfRequired
*
*  Function description
*    Initialize medium if it has not already been initialized.
*
*  Parameters
*    pDevice     - Pointer to a device driver structure.
*
*/
int FS_LB_InitMediumIfRequired(FS_DEVICE * pDevice) {
  if (pDevice->Data.IsInited == 0) {
    FS_LB_InitMedium(pDevice);
  }
  return pDevice->Data.IsInited;
}


/*********************************************************************
*
*       FS_LB_ReadDevice
*
*  Function description
*    Read sector from device. It also checks whether the sector can be
*    read from the cache if available.
*
*  Parameters
*    pDriver     - Pointer to a device driver structure.
*    Sector      - Physical sector to be read from the device.
*                  The Partion start sector is not added.
*    pBuffer     - Pointer to buffer for storing the data.
*    Type        - The type of sector that shall be read.
*
*  Return value
*    ==0         - Sector has been read and copied to pBuffer.
*    < 0         - An error has occured.
*/
int FS_LB_ReadDevice(FS_DEVICE *pDevice, U32 Sector, void *pBuffer, U8 Type) {
  int x;

  INC_READ_SECTOR_CNT();             /* For statistics / debugging only */
  FS_DEBUG_LOGF(("Read          %s:%d: %s Sector: 0x%.8X", pDevice->pType->pfGetName(pDevice->Data.Unit), pDevice->Data.Unit, _Type2Name(Type), Sector));
  SET_BUSY_LED(pDevice);
#if FS_SUPPORT_CACHE
  {
    const FS_DEVICE_DATA * pDevData;
    void                 * pCacheData;
    const FS_CACHE_API   * pCacheAPI;
    pDevData   = &pDevice->Data;
    pCacheData = pDevData->pCacheData;
    pCacheAPI  = pDevData->pCacheAPI;
    if (pCacheAPI) {
      if ((pCacheAPI->pfReadFromCache)(pCacheData, Sector, pBuffer, Type)) {
        x = _Read(pDevice, Sector, pBuffer, 1, Type);
      } else {
        FS_DEBUG_LOGF((" (cache)"));
        x = 0;
      }
    } else {
      x = _Read(pDevice, Sector, pBuffer, 1, Type);
    }
  }
#else
    FS_USE_PARA(Type);
    x = _Read(pDevice, Sector, pBuffer, 1, Type);
#endif
  CLR_BUSY_LED(pDevice);
  FS_DEBUG_LOGF(("\n"));
  return  x;
}

/*********************************************************************
*
*       FS_LB_ReadPart
*
*  Function description
*    Read sector from volume.
*
*  Parameters
*    pDriver     - Pointer to a device driver structure.
*    Sector      - Physical sector to be read from the partition.
*                  The Partion start sector is added.
*    pBuffer     - Pointer to buffer for storing the data.
*    Type        - The type of sector that shall be read.
*
*  Return value
*    ==0         - Sector has been read and copied to pBuffer.
*    <0          - An error has occured.
*/
int FS_LB_ReadPart(FS_PARTITION *pPart, U32 Sector, void *pBuffer, U8 Type) {
  return FS_LB_ReadDevice(&pPart->Device, pPart->StartSector + Sector, pBuffer, Type);
}

#if FS_PREFETCH_FAT
// 对FAT簇管理扇区进行预取. 
// 老版本读取时仅读取一个扇区,花费的代价太大, 改为一次读取多个连续的扇区到CACHE中去.

int    FS_LB_ReadPartPrefetch   (  FS_PARTITION * pPart,   U32 Sector,      U32 SectorLimit,      void * pBuffer, U8 Type){
  return FS_LB_ReadDevicePrefetch(&pPart->Device, pPart->StartSector + Sector, pPart->StartSector + SectorLimit, pBuffer, Type);	
}

int FS_LB_ReadDevicePrefetch(FS_DEVICE *pDevice, U32 Sector, U32 SectorLimit, void *pBuffer, U8 Type) {
  int x;

  INC_READ_SECTOR_CNT();             /* For statistics / debugging only */
  FS_DEBUG_LOGF(("Read          %s:%d: %s Sector: 0x%.8X", pDevice->pType->pfGetName(pDevice->Data.Unit), pDevice->Data.Unit, _Type2Name(Type), Sector));
  SET_BUSY_LED(pDevice);
#if FS_SUPPORT_CACHE
  {
    const FS_DEVICE_DATA * pDevData;
    void                 * pCacheData;
    const FS_CACHE_API   * pCacheAPI;
    pDevData   = &pDevice->Data;
    pCacheData = pDevData->pCacheData;
    pCacheAPI  = pDevData->pCacheAPI;
    if (pCacheAPI) 
	 {
      if ((pCacheAPI->pfReadFromCache)(pCacheData, Sector, pBuffer, Type)) 
		{
		  // cache读取失败
			
		  // 检查cache预取接口是否存在
		  if(pCacheAPI->pfPrefetch)
		  {
			 // cache预取接口存在
			 // 从磁盘预取数据 
			 if ((pCacheAPI->pfPrefetch)(pDevice, Sector, SectorLimit, Type) == 0)	
			 {
				 // 预取成功
				 // 将读取的内容从Cache复制到读取缓冲区
				 if ((pCacheAPI->pfReadFromCache)(pCacheData, Sector, pBuffer, Type) == 0)
					x = 0;
				 else
				   x = -1;
			 }
			 else
			 {
				 // 预取失败
				 x = -1;
			 }
		  }
		  else
		  {
			 // cache预取接口不存在   
			 x = _Read(pDevice, Sector, pBuffer, 1, Type);
		  }
      } 
		else 
		{
        FS_DEBUG_LOGF((" (cache)"));
        x = 0;
      }
    } 
	 else 
	 {
      x = _Read(pDevice, Sector, pBuffer, 1, Type);
    }
  }
#else
  FS_USE_PARA(Type);
  x = _Read(pDevice, Sector, pBuffer, 1, Type);
#endif
  CLR_BUSY_LED(pDevice);
  FS_DEBUG_LOGF(("\n"));
  return  x;
}

#endif

/*********************************************************************
*
*       FS_LB_ReadBurst
*
*  Function description
*    Read multiple sectors from device.
*
*  Parameters
*    pDriver     - Pointer to a device driver structure.
*    Unit        - Unit number.
*    SectorNo    - First sector to be read from the device.
*    NumSectors  - Number of sectors to be read from the device.
*    pBuffer     - Pointer to buffer for storing the data.
*    Type        - The type of sector that shall be read.
*
*  Return value
*    ==0         - Sectors have been read and copied to pBuffer.
*    <0          - An error has occured.
*/
int FS_LB_ReadBurst(FS_PARTITION *pPart, U32 SectorNo, U32 NumSectors, void *pBuffer, U8 Type) {
  int x = 0;
  FS_DEVICE      * pDevice;

  pDevice     = &pPart->Device;
  SectorNo   += pPart->StartSector;
  INC_READ_BURST_CNT();                    /* For statistics / debugging only */
  INC_READ_BURST_SECTOR_CNT(NumSectors);   /* For statistics / debugging only */
  FS_DEBUG_LOGF(("ReadBurst     %s:%d: %s Sector: 0x%.8X, NumSectors: %d", pDevice->pType->pfGetName(pDevice->Data.Unit), pDevice->Data.Unit, _Type2Name(Type), SectorNo, NumSectors));
  SET_BUSY_LED(pDevice);
#if FS_SUPPORT_CACHE
  {
    const FS_DEVICE_DATA * pDevData;
    void                 * pCacheData;
    const FS_CACHE_API   * pCacheAPI;
    U32                 NumSectors2Read;
    U32                 FirstSector;
    U16                 SectorSize;
    char                   NeedReadBurst;
    void                 * pReadBuffer;
    NeedReadBurst   = 0;
    NumSectors2Read = 0;
    FirstSector     = 0;
    pReadBuffer     = NULL;
    pDevData        = &pDevice->Data;
    pCacheData      = pDevData->pCacheData;
    pCacheAPI       = pDevData->pCacheAPI;
    SectorSize      = FS_GetSectorSize(pDevice);
    if (pCacheAPI) {
      U8 * p;
      char    r;
      p = (U8 *)pBuffer;
      do {
        r = (pCacheAPI->pfReadFromCache)(pCacheData, SectorNo, p, Type);
        if (r) {
          /* Cache miss. We need to read from hardware. Since we try to use burst mode, we do not read immediately */
          if (NeedReadBurst) {
            NumSectors2Read++;
          } else {
            FirstSector     = SectorNo;
            pReadBuffer     = p;
            NumSectors2Read = 1;
            NeedReadBurst   = 1;
          }
        } else {
          if (NeedReadBurst) {
            NeedReadBurst   = 0;
            x = _Read(pDevice, FirstSector, pReadBuffer, NumSectors2Read, Type);
            if (x) {
              break;                     /* End read operation because of read failure */
            }
          }
        }
        p  += SectorSize;
        SectorNo++;
      } while(--NumSectors);
      /*
       * End of read routine reached. There may be a hardware "read burst" operation pending, which needs to be executed in this case.
       */
      if (NeedReadBurst) {
        x = _Read(pDevice, FirstSector, pReadBuffer, NumSectors2Read, Type);
      }
    } else {
      x = _Read(pDevice, SectorNo, pBuffer, NumSectors, Type);
    }
  }
#else
  FS_USE_PARA(Type);
  x = _Read(pDevice, SectorNo, pBuffer, NumSectors, Type);
#endif
  CLR_BUSY_LED(pDevice);
  FS_DEBUG_LOGF(("\n"));
  return  x;
}

/*********************************************************************
*
*       FS_LB_WriteBurst
*
*  Function description
*    Write multiple sectors to device.
*
*  Parameters
*    pDriver     - Pointer to a device driver structure.
*    Unit        - Unit number.
*    SectorNo    - First sector to be written to the device.
*    NumSectors  - Number of sectors to be written.
*    pBuffer     - Pointer to buffer for holding the data.
*    Type        - The type of sector that shall be written.
*
*  Return value
*    ==0         - Sectors have been read and copied to pBuffer.
*    <0          - An error has occured.
*/
int FS_LB_WriteBurst(FS_PARTITION *pPart, U32 SectorNo, U32 NumSectors, const void *pBuffer, U8 Type) {
  int         x;
  FS_DEVICE * pDevice;

  FS_USE_PARA(Type);
  pDevice     = &pPart->Device;
  SectorNo   += pPart->StartSector;
  INC_WRITE_BURST_CNT();                       /* For statistics / debugging only */
  INC_WRITE_BURST_SECTOR_CNT(NumSectors);      /* For statistics / debugging only */
  FS_DEBUG_LOGF(("WriteBurst    %s:%d: %s Sector: 0x%.8X, NumSectors: %d", pDevice->pType->pfGetName(pDevice->Data.Unit), pDevice->Data.Unit, _Type2Name(Type), SectorNo, NumSectors));
  SET_BUSY_LED(pDevice);
  x = _Write(pDevice, SectorNo, pBuffer, NumSectors, 0);
  CLR_BUSY_LED(pDevice);
#if FS_SUPPORT_CACHE
  if (x == 0) {
    const FS_DEVICE_DATA * pDevData;
    const FS_CACHE_API   * pCacheAPI;
    U16                 SectorSize;
    pDevData   = &pDevice->Data;
    pCacheAPI  = pDevData->pCacheAPI;
    SectorSize = FS_GetSectorSize(pDevice);
    if (pCacheAPI) {
      do {
        U8 * p;
        /* ToDo: Return value of write cache is ignored now. Optimization possible */
        if ((pDevData->pCacheAPI->pfUpdateCache)(pDevice, SectorNo++, pBuffer, Type)) {
          FS_DEBUG_WARN("Could not update sector in cache");
        }
        p = (U8 *)pBuffer;
        pBuffer = p + SectorSize;
      } while(--NumSectors);
    }
  }
#endif
  FS_DEBUG_LOGF(("\n"));
  return  x;
}

/*********************************************************************
*
*       FS_LB_WriteMultiple
*
*  Function description
*    Write multiple sectors to device.
*
*  Parameters
*    pDriver     - Pointer to a device driver structure.
*    Unit        - Unit number.
*    SectorNo    - First sector to be written to the device.
*    NumSectors  - Number of sectors to be written.
*    pBuffer     - Pointer to buffer for holding the data.
*    Type        - The type of sector that shall be written.
*
*  Return value
*    ==0         - Sectors have been read and copied to pBuffer.
*    <0          - An error has occured.
*/
int FS_LB_WriteMultiple(FS_PARTITION *pPart, U32 SectorNo, U32 NumSectors, const void *pBuffer, U8 Type) {
  int         x;
  FS_DEVICE * pDevice;

  FS_USE_PARA(Type);
  pDevice     = &pPart->Device;
  SectorNo   += pPart->StartSector;
  INC_WRITE_MULTIPLE_CNT();                         /* For statistics / debugging only */
  INC_WRITE_MULTIPLE_SECTOR_CNT(NumSectors);        /* For statistics / debugging only */
  FS_DEBUG_LOGF(("WriteMultiple %s:%d: %s Sector: 0x%.8X, NumSectors: %d", pDevice->pType->pfGetName(pDevice->Data.Unit), pDevice->Data.Unit, _Type2Name(Type), SectorNo, NumSectors));
  SET_BUSY_LED(pDevice);
  x = _Write(pDevice, SectorNo, pBuffer, NumSectors, 1);
  CLR_BUSY_LED(pDevice);
#if FS_SUPPORT_CACHE
  if (x == 0) {
    const FS_DEVICE_DATA * pDevData;
    const FS_CACHE_API   * pCacheAPI;
    pDevData   = &pDevice->Data;
    pCacheAPI  = pDevData->pCacheAPI;
    if (pCacheAPI) {
      do {
        /* ToDo: Return value of write cache is ignored now. Optimization possible */
        if (pDevData->pCacheAPI->pfUpdateCache(pDevice, SectorNo++, pBuffer, Type)) {
          FS_DEBUG_WARN("Could not update sector in cache");
        }
      } while(--NumSectors);
    }
  }
#endif
  FS_DEBUG_LOGF(("\n"));
  return  x;
}
/*********************************************************************
*
*       FS_LB_WriteDevice
*
*  Function description
*    FS internal function. Write sector to device.
*
*  Parameters
*    pDriver     - Pointer to a device driver structure.
*    Unit        - Unit number.
*    Sector      - Sector to be written to the device.
*    pBuffer     - Pointer to data to be stored.
*    Type        - The type of sector that shall be written.
*
*  Return value
*    == 0        - Sector has been written to the device.
*    <  0        - An error has occured.
*/
#if FS_VERIFY_WRITE
static U8 _acVerifyBuffer[FS_MAX_SECTOR_SIZE];
#endif

int FS_LB_WriteDevice(FS_DEVICE *pDevice, U32 Sector, const void *pBuffer, U8 Type) {
  char IsWritten = 0;

  FS_USE_PARA(Type);
  INC_WRITE_SECTOR_CNT();             /* For statistics / debugging only */
  FS_DEBUG_LOGF(("Write         %s:%d: %s Sector: 0x%.8X", pDevice->pType->pfGetName(pDevice->Data.Unit), pDevice->Data.Unit, _Type2Name(Type), Sector));
#if FS_SUPPORT_CACHE
  {
    const FS_DEVICE_DATA * pDevData;
    const FS_CACHE_API   * pCacheAPI;
    pDevData   = &pDevice->Data;
    pCacheAPI  = pDevData->pCacheAPI;
    if (pCacheAPI) {
      IsWritten = (pCacheAPI->pfWriteIntoCache)(pDevice, Sector, pBuffer, Type);
      if (IsWritten) {
        FS_DEBUG_LOGF((" (cache)"));
      }
    }
  }
#endif
  if (IsWritten == 0) {
    SET_BUSY_LED(pDevice);
    if (_Write(pDevice, Sector, pBuffer, 1, 0) == 0) {
      IsWritten = 1;
    } else {
      FS_DEBUG_ERROROUT1("Failed to write Sector: ", Sector);
    }
#if FS_VERIFY_WRITE
    /* In higher debug levels, read medium once more to verify write operation */
    {
      U16 SectorSize;
      SectorSize= FS_GetSectorSize(pDevice);
      _Read(pDevice, Sector, &_acVerifyBuffer[0], 1, Type);
      if (FS_MEMCMP(pBuffer, (const void *)&_acVerifyBuffer[0], SectorSize)) {
        FS_DEBUG_ERROROUT1("Verify failed after write. Sector 0x", Sector);
      }
    }
#endif
    CLR_BUSY_LED(pDevice);
  }
  FS_DEBUG_LOGF(("\n"));
  return IsWritten ? 0 : -1;
}

/*********************************************************************
*
*         FS_LB_WritePart
*
*  Function description
*    Read sector from volume.
*
*  Parameters
*    pDriver     - Pointer to a device driver structure.
*    Sector      - Physical sector to be written to the partition.
*                  The Partion start sector is added.
*    pBuffer     - Pointer to buffer for storing the data.
*    Type        - The type of sector that shall be written.
*
*  Return value
*    ==0         - Sector has been read and copied to pBuffer.
*    <0          - An error has occured.
*/
int FS_LB_WritePart(FS_PARTITION *pPart, U32 Sector, const void *pBuffer, U8 Type) {
  return FS_LB_WriteDevice(&pPart->Device, pPart->StartSector + Sector, pBuffer, Type);
}


/*********************************************************************
*
*       FS_LB_Ioctl
*
*  Function description
*    Executes device command.
*
*  Parameters
*    pDriver     - Pointer to a device driver structure.
*    Cmd         - Command to be executed.
*    Aux         - Parameter depending on command.
*    pBuffer     - Pointer to a buffer used for the command.
*
*  Return value
*    Command specific. In general a negative value means an error.
*/
int FS_LB_Ioctl(FS_DEVICE * pDevice, I32 Cmd, I32 Aux, void *pBuffer) {
  int x;
  const FS_DEVICE_TYPE * pDeviceType;

  FS_LB_InitMediumIfRequired(pDevice);
  pDeviceType = pDevice->pType;
  x = (pDeviceType->pfIoCtl)(pDevice->Data.Unit, Cmd, Aux, pBuffer);
  return x;
}

/*********************************************************************
*
*       FS_GetSectorSize
*
*  Function description
*    Returns the sector size of a device.
*
*/
U16 FS_GetSectorSize(const FS_DEVICE * pDevice) {
  U16 r = 0;
  FS_DEV_INFO DevInfo;

  if (pDevice->pType->pfIoCtl(pDevice->Data.Unit, FS_CMD_GET_DEVINFO, 0, &DevInfo) == 0) {
    r = DevInfo.BytesPerSector;
  }
  return r;
}

/*********************************************************************
*
*       FS_LB_GetDeviceInfo
*
*/
int FS_LB_GetDeviceInfo(const FS_DEVICE * pDevice, FS_DEV_INFO * pDevInfo) {
  int r;

  r = 0;
  if (pDevice->pType->pfIoCtl(pDevice->Data.Unit, FS_CMD_GET_DEVINFO, 0, (void *)pDevInfo)) {
    r = -1;
  }
  return r;
}

/*********************************************************************
*
*       FS_LB_FreeSectors
*
*  Function description
*    Frees unused sectors (from cache and devices) of a partition
*
*/
void FS_LB_FreePartSectors(FS_PARTITION* pPart, U32 SectorIndex, U32 NumSectors) {
  FS_DEVICE * pDevice;

  pDevice = &pPart->Device;
  SectorIndex += pPart->StartSector;      /* Convert into device sector index */
  FS_USE_PARA(pDevice);
  FS_DEBUG_LOGF(("FreeSectors   %s:%d:      Sector: 0x%.8X, NumSectors:  0x%.8x", pDevice->pType->pfGetName(pDevice->Data.Unit), pDevice->Data.Unit, SectorIndex, NumSectors));
#if FS_SUPPORT_JOURNAL
  if (pDevice->Data.JournalIsActive) {
//    r = FS_JOURNAL_FreeSector(pDevice, SectorNo, pBuffer, NumSectors, RepeatSame);
// TBD: This can be optimized if the journal can also manage "freeed" sectors.
  } else
#endif
  {
    FS_LB_Ioctl(pDevice, FS_CMD_FREE_SECTORS, SectorIndex, &NumSectors);
  }

  #if FS_SUPPORT_CACHE
  {
    CACHE_FREE CacheFree;

    CacheFree.FirstSector = SectorIndex;
    CacheFree.NumSectors  = NumSectors;
    FS__CACHE_CommandDeviceNL(pDevice, FS_CMD_CACHE_FREE_SECTORS, &CacheFree);
  }
  #endif
  FS_DEBUG_LOGF(("\n"));
}

#if FS_DEBUG_LEVEL >= FS_DEBUG_LEVEL_CHECK_PARA
/*********************************************************************
*
*       FS_LB_GetCounters
*
*/
void FS_LB_GetCounters(FS_STAT * pStat) {
  *pStat = _Stat;
}

/*********************************************************************
*
*       FS_LB_ResetCounters
*
*/
void FS_LB_ResetCounters(void) {
  FS_MEMSET(&_Stat, 0, sizeof(FS_STAT));
}
#endif


/*************************** End of file ****************************/
