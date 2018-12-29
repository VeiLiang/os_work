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
File        : FS_Core.c
Purpose     : File system's Core routines
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdlib.h>

#include "FS_Int.h"

/*********************************************************************
*
*       #define constants
*
**********************************************************************
*/

/*********************************************************************
*
*       Local data types
*
**********************************************************************
*/
typedef struct {
  int   InUse;
  U32 * pBuffer;
} SECTOR_BUFFER;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static SECTOR_BUFFER * _paSectorBuffer;
static unsigned        _NumSectorBuffers;
static char            _IsInited;

/*********************************************************************
*
*       Public data
*
**********************************************************************
*/
U32 FS__MaxSectorSize = 512;

/*********************************************************************
*
*       _SB_Clean
*/
static void _SB_Clean(FS_SB * pSB) {
  if (pSB->HasError) {
    return;       /* Previous error, do not continue */
  }
  if (pSB->IsDirty) {
    FS_LB_WritePart(pSB->pPart, pSB->SectorNo, pSB->pBuffer, pSB->Type);
    /* Handle the optional sector copy (Typically used for the second FAT) */
#if FS_MAINTAIN_FAT_COPY
    if (pSB->WriteCopyOff) {
      FS_LB_WritePart(pSB->pPart, pSB->SectorNo + pSB->WriteCopyOff, pSB->pBuffer, pSB->Type);
    }
#endif
    pSB->IsDirty = 0;
  }
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       FS__DivideU32Up
*/
U32 FS__DivideU32Up(U32 Nom, U32 Div) {
  return (Nom + Div - 1) / Div;
}

/*********************************************************************
*
*       FS__strchr
*
*  Description:
*    FS internal function. Locate the first occurrence of c (converted
*    to a char) in the string pointed to by s.
*
*  Parameters:
*    s           - Pointer to a zero terminated string.
*    c           - 'Character' value to find.
*
*  Return value:
*    NULL        - c was not found
*    != NULL     - Pointer to the located character in s.
*/
const char * FS__strchr(const char *s, int c) {
  const char ch = c;

  for (; *s != ch; ++s) {
    if (*s == '\0') {
      return (const char *)0;
    }
  }
  return s;
}


/*********************************************************************
*
*       FS__AllocSectorBuffer
*
*  Description:
*    Allocate a sector buffer.
*
*  Return value:
*    ==0         - Cannot allocate a buffer.
*    !=0         - Address of a buffer.
*/
U8 * FS__AllocSectorBuffer(void) {
  unsigned i;
  U8 * r;

  r = (U8*)NULL;
  FS_LOCK_SYS();
  for (i = 0; i < _NumSectorBuffers; i++) {
    if (_paSectorBuffer[i].InUse == 0) {
      _paSectorBuffer[i].InUse  = 1;
      r = (U8 *)_paSectorBuffer[i].pBuffer;
      break;
    }
  }
  if (r == (U8 *)NULL) {
    FS_DEBUG_ERROROUT("No sector buffer available");
  }
  FS_UNLOCK_SYS();
  return r;
}

/*********************************************************************
*
*       FS__FreeSectorBuffer
*
*  Description:
*    Free sector buffer.
*
*  Parameters:
*    pBuffer     - Pointer to a buffer, which has to be set free.
*/
void FS__FreeSectorBuffer(void *pBuffer) {
  unsigned i;
  FS_LOCK_SYS();
  for (i = 0; i < _NumSectorBuffers; i++) {
    if (((void*)_paSectorBuffer[i].pBuffer) == pBuffer) {
      _paSectorBuffer[i].InUse = 0;
      break;
    }
  }
  FS_UNLOCK_SYS();
}

/*********************************************************************
*
*       FS__SB_Flush
*/
void FS__SB_Flush(FS_SB * pSB) {
    FS_PARTITION * pPart;
    FS_DEVICE *pDevice;

    pSB->IsDirty = 1;
    _SB_Clean(pSB);

    pPart = pSB->pPart;
    pDevice = &pPart->Device;	
    pDevice->pType->pfIoCtl(pDevice->Data.Unit, FS_CMD_FLASH_WRITE_BACK, 0, 0);	//add for nand
}

/*********************************************************************
*
*       FS__SB_Create
*
*  Function description
*    Creates a Smart buffer
*/
char FS__SB_Create(FS_SB * pSB, FS_PARTITION * pPart) {
  U8   * pBuffer;

  FS_MEMSET(pSB, 0, sizeof(FS_SB));
  pBuffer = FS__AllocSectorBuffer();
  if (pBuffer == NULL) {
    FS_DEBUG_ERROROUT("FS_SB_Create: No buffer");
    return 1;               /* Error, no buffer */
  }
  pSB->pBuffer  = pBuffer;
  pSB->pPart    = pPart;
  pSB->SectorNo = 0xFFFFFFFF;
  pSB->Type     = FS_SB_TYPE_DATA;
  return 0;               /* O.K. */
}

/*********************************************************************
*
*       FS__SB_Delete
*/
void FS__SB_Delete(FS_SB * pSB) {
  _SB_Clean(pSB);
  FS__FreeSectorBuffer(pSB->pBuffer);
}

/*********************************************************************
*
*       FS__SB_Clean
*
*  Function description
*    Cleans the smart buffer: If the buffer is marked as being dirty,
*    it is written.
*/
void FS__SB_Clean(FS_SB * pSB) {
  _SB_Clean(pSB);
}
/*********************************************************************
*
*       FS__SB_MarkDirty
*/
void FS__SB_MarkDirty(FS_SB * pSB) {
  pSB->IsDirty = 1;
}

/*********************************************************************
*
*       FS__SB_SetWriteCopyOff
*
*  Function description
*    Sets the "WriteCopyOffset", which is the offset of the sector to write a copy to.
*    Typically used for FAT  sectors only.
*/
#if FS_MAINTAIN_FAT_COPY
void FS__SB_SetWriteCopyOff(FS_SB * pSB, U32 Off) {
  pSB->WriteCopyOff = Off;
}
#endif

/*********************************************************************
*
*       FS__SB_SetSector
*
*  Function description
*    Assigns a sector to a smart buffer.
*
*/
void FS__SB_SetSector(FS_SB * pSB, U32 SectorNo, U8 Type) {
  if (SectorNo != pSB->SectorNo) {
    if (pSB->IsDirty) {
      _SB_Clean(pSB);
    }
    pSB->SectorNo = SectorNo;
    pSB->Type     = Type;
    pSB->Read     = 0;
#if FS_MAINTAIN_FAT_COPY
    pSB->WriteCopyOff = 0;
#endif
  }
}

/*********************************************************************
*
*       FS__SB_MarkValid
*
*  Function description
*    Marks a buffer as containing valid sector data.
*    Useful if a buffer is filled and needs to be written later on.
*/
void FS__SB_MarkValid(FS_SB * pSB, U32 SectorNo, U8 Type) {
  FS__SB_SetSector(pSB, SectorNo, Type);
  pSB->Read     = 1;
  pSB->IsDirty  = 1;
}


/*********************************************************************
*
*       FS__SB_MarkNotDirty
*
*  Function description
*    Marks a buffer as containing valid sector data.
*    Useful if a buffer is filled and needs to be written later on.
*/
void FS__SB_MarkNotDirty(FS_SB * pSB) {
  pSB->IsDirty  = 0;
}


/*********************************************************************
*
*       FS__SB_Read
*
*  Return value
*    0    O.K.
*    !=0  Error
*/
char FS__SB_Read(FS_SB * pSB) {
  if (pSB->HasError) {
    return 1;       /* Previous error, do not continue */
  }
  if ((pSB->Read) == 0) {
	  
#if FS_PREFETCH_FAT
	 // 对FAT簇管理扇区进行预取. 
	 // 老版本读取时仅读取一个扇区,花费的代价太大, 改为一次读取多个连续的扇区到CACHE中去.
	 char ret;
    if(pSB->Type == FS_SB_TYPE_MANAGEMENT)
		ret = FS_LB_ReadPartPrefetch (pSB->pPart, pSB->SectorNo, pSB->SectorLimit, pSB->pBuffer, pSB->Type);
	 else
		ret = FS_LB_ReadPart(pSB->pPart, pSB->SectorNo, pSB->pBuffer, pSB->Type);
	 if(ret) {
      pSB->HasError = 1;
      return 1;     /* Read failed */
    } else {
      pSB->Read = 1;
    }
#else	  
    if (FS_LB_ReadPart(pSB->pPart, pSB->SectorNo, pSB->pBuffer, pSB->Type)) {
      pSB->HasError = 1;
      return 1;     /* Read failed */
    } else {
      pSB->Read = 1;
    }
#endif
  }
  return 0;       /* Sector read successfully */
}

/*********************************************************************
*
*       FS__SB_Write
*
*  Return value
*    0    O.K.
*    !=0  Error
*/
char FS__SB_Write(FS_SB * pSB) {
  FS_DEBUG_ASSERT(pSB->SectorNo != 0xFFFFFFFF);
  if (pSB->HasError) {
    return 1;       /* Previous error, do not continue */
  }
  if (FS_LB_WritePart(pSB->pPart, pSB->SectorNo, pSB->pBuffer, pSB->Type)) {
    pSB->HasError = 1;
    return 1;     /* Write failed */
  }
  pSB->IsDirty = 0;
  return 0;       /* Sector written successfully */
}


/*********************************************************************
*
*       FS__LocatePartition
*         _LocatePartition (static helper)
*
*  Return value
*    0    O.K.
*    !=0  Error
*/
static signed char _LocatePartition(FS_VOLUME * pVolume, U8* pBuffer) {
  U32 StartSector;
  U32 NumSectors;
  /* Calculate start sector of the first partition */
  StartSector = FS__GetMediaStartSecEx(pVolume, &NumSectors, pBuffer);
  if (StartSector == 0xFFFFFFFF) { /* check if MBR / BPB was invalid */
    return -1;                     /* Invalid MBR / BPB */
  }
  pVolume->Partition.StartSector = StartSector;
  pVolume->Partition.NumSectors  = NumSectors;
  return 0;
}

signed char FS__LocatePartition(FS_VOLUME * pVolume) {
  signed char r;
  U8 * pBuffer;
  U16  BytesPerSector;

  BytesPerSector = FS_GetSectorSize(&pVolume->Partition.Device);
  pBuffer = FS__AllocSectorBuffer();
  /*
   * Check if the a sector fits into the sector buffer
   */
  if ((BytesPerSector > FS__MaxSectorSize) || (BytesPerSector == 0)) {
    FS_DEBUG_ERROROUT1("FS_LocatePartition: Invalid BytesPerSector value: ", BytesPerSector);
    r = -1;
  } else {
    r = _LocatePartition(pVolume, pBuffer);
  }
  FS__FreeSectorBuffer(pBuffer);
  return r;
}

/*********************************************************************
*
*       FS_SetBusyLEDCallback
*
*/
void FS_SetBusyLEDCallback(const char * sVolumeName, FS_BUSY_LED_CALLBACK * pfBusyLEDCallback) {
#if FS_SUPPORT_BUSY_LED
  FS_VOLUME * pVolume;
  FS_LOCK();
  if (sVolumeName) {
    pVolume = FS__FindVolume(sVolumeName, NULL);
    FS_LOCK_SYS();
    if (pVolume) {
      pVolume->Partition.Device.Data.pfSetBusyLED = pfBusyLEDCallback;
    }
    FS_UNLOCK_SYS();
  }
  FS_UNLOCK();
#else
  FS_DEBUG_WARN("FS_SetBusyLEDCallback() has no function because FS_SUPPORT_BUSY_LED is disabled.");
#endif
}

/*********************************************************************
*
*       FS_SetMemAccessCallback
*
*/
void FS_SetMemAccessCallback(const char * sVolumeName, FS_MEMORY_IS_ACCESSIBLE_CALLBACK * pfIsAccessibleCallback) {
#if FS_SUPPORT_CHECK_MEMORY
  FS_VOLUME * pVolume;
  FS_LOCK();
  if (sVolumeName) {
    pVolume = FS__FindVolume(sVolumeName, NULL);
    FS_LOCK_SYS();
    if (pVolume) {
      pVolume->Partition.Device.Data.pfMemoryIsAccessible = pfIsAccessibleCallback;
    }
    FS_UNLOCK_SYS();
  }
  FS_UNLOCK();
#else
  FS_DEBUG_WARN("FS_SetMemAccessCallback() has no function because FS_SUPPORT_CHECK_MEMORY is disabled.");
#endif
}


/*********************************************************************
*
*       FS_LoadU16BE
*
*  Function description:
*    Reads a 16 bit value stored in big endian format from a byte array.
*/
U16 FS_LoadU16BE(const U8 *pBuffer) {
  U16 r;
  r = *pBuffer++;
  r = (r << 8) | *pBuffer;
  return r;
}

/*********************************************************************
*
*       FS_LoadU32BE
*
*  Function description:
*    Reads a 32 bit value stored in big endian format from a byte array.
*/
U32 FS_LoadU32BE(const U8 *pBuffer) {
  U32 r;
  r = *pBuffer++;
  r = (r << 8) | *pBuffer++;
  r = (r << 8) | *pBuffer++;
  r = (r << 8) | *pBuffer;
  return r;
}

/*********************************************************************
*
*       FS_StoreU16BE
*
*  Function description:
*    Stores a 16 bit value in big endian format into a byte array.
*/
void FS_StoreU16BE(U8 *pBuffer, unsigned Data) {
  *pBuffer++ = (U8)(Data >> 8);
  *pBuffer   = (U8) Data;
}

/*********************************************************************
*
*       FS_StoreU32BE
*
*  Function description:
*    Stores a 32 bit value in big endian format into a byte array.
*/
void FS_StoreU32BE(U8 *pBuffer, U32 Data) {
  *pBuffer++ = (U8)(Data >> 24);
  *pBuffer++ = (U8)(Data >> 16);
  *pBuffer++ = (U8)(Data >> 8);
  *pBuffer   = (U8) Data;
}


/*********************************************************************
*
*       FS_LoadU32LE
*
*  Function description:
*    Reads a 32 bit little endian from a char array.
*
*  Parameters:
*    pBuffer     - Pointer to a char array.
*
*  Return value:
*    result      - The value as U32 data type
*
*/
U32 FS_LoadU32LE(const U8 *pBuffer) {
  U32 r;
  r = (U32)pBuffer[3] & 0x000000FF;
  r <<= 8;
  r += (U32)pBuffer[2] & 0x000000FF;
  r <<= 8;
  r += (U32)pBuffer[1] & 0x000000FF;
  r <<= 8;
  r += (U32)pBuffer[0] & 0x000000FF;
  return r;
}

/*********************************************************************
*
*       FS_StoreU32LE
*
*  Function description:
*    Stores 32 bits little endian into memory.
*/
void FS_StoreU32LE(U8 *pBuffer, U32 Data) {
  *pBuffer++ = (U8)Data;
  Data >>= 8;
  *pBuffer++ = (U8)Data;
  Data >>= 8;
  *pBuffer++ = (U8)Data;
  Data >>= 8;
  *pBuffer   = (U8)Data;
}

/*********************************************************************
*
*       FS_StoreU24LE
*
*  Function description:
*    Stores 24 bits little endian into memory.
*/
void FS_StoreU24LE(U8 *pBuffer, U32 Data) {
  *pBuffer++ = (U8)Data;
  Data >>= 8;
  *pBuffer++ = (U8)Data;
  Data >>= 8;
  *pBuffer = (U8)Data;
}

/*********************************************************************
*
*       FS_StoreU16LE
*
*  Function description:
*    Writes 16 bit little endian.
*/
void FS_StoreU16LE(U8 *pBuffer, unsigned Data) {
  *pBuffer++ = (U8)Data;
  Data >>= 8;
  *pBuffer = (U8)Data;
}

/*********************************************************************
*
*       FS_LoadU16LE
*
*  Function description:
*    Reads a 16 bit little endian from a char array.
*
*  Parameters:
*    pBuffer     - Pointer to a char array.
*
*  Return value:
*    The value as U16 data type
*
*  Notes
*    (1) This cast should not be necessary, but on some compilers (NC30)
*        it is required in higher opt. levels since otherwise the
*        argument promotion to integer size is skipped, leading to wrong result of 0.
*
*/
U16 FS_LoadU16LE(const U8 *pBuffer) {
  U16 r;
  r = (U16)(*pBuffer | ((unsigned)*(pBuffer + 1) << 8));
  return r;
}


/*********************************************************************
*
*       FS_Alloc
*
*  Function description
*    As the name indicates, this function provides memory to the File system.
*
*  Notes
*    (1)  Fragmentation
*         The file system allocates memory only in the configuration phase, not during
*         normal operation, so that fragmentation should not occur.
*    (2)  Failure
*         Since the memory is required for proper operation of the file system,
*         this function does not return on failure.
*         In case of a configuration problem where insufficient memory is available
*         to the application, this is normally detected by the programmer in the debug phase.
*
*/
void * FS_Alloc(I32 NumBytes) {
  void * p;

  p = FS_X_Alloc(NumBytes);
  if (!p) {
    FS_DEBUG_ERROROUT("Could not allocate memory!");
#if FS_DEBUG_LEVEL >= FS_DEBUG_LEVEL_CHECK_PARA
    FS_X_Panic(FS_ERROR_ALLOC);
#endif
    while (1);                // Do NOT remove since the file system assumes allocation to work.
  }
  return p;
}

/*********************************************************************
*
*       FS_AllocZeroed
*/
void * FS_AllocZeroed(I32 NumBytes) {
  void * p;

  p = FS_Alloc(NumBytes);
  FS_MEMSET(p, 0, NumBytes);           // Note: p must be valid, no need to check.
  return p;
}

/*********************************************************************
*
*       FS_AllocZeroedPtr
*
*  Function description
*    Makes sure that zeroed memory is allocated to the specified pointer.
*    If pointer is NULL, memory is allocated and pointer is updated.
*    In either case memory is zeroed.
*
*/
void FS_AllocZeroedPtr(void ** pp, I32 NumBytes) {
  void * p;

  p = *pp;
  if (p == NULL) {
    p   = FS_Alloc(NumBytes);
    *pp = p;
  }
  FS_MEMSET(p, 0, NumBytes);    // Note: p must be valid, no need to check.
}



/*********************************************************************
*
*       FS_STORAGE_Init
*
*  Function description:
*    This function only intializes the driver and OS if necessary.
*    It sotres then the information of the drivers in FS__aVolume.
*    This allows to use the file system as a pure sector read/write
*    software. This can be useful when using the file system as
*    USB mass storage client driver.
*
*  Return value:
*    The return value is used to tell the high level init how many
*    drivers can be used at the same time. The function will accordinaly
*    allocate the sectors buffers that are neccessary for a FS operation.
*
*/
unsigned FS_STORAGE_Init(void) {
  unsigned NumDriverLocks;
  unsigned NumLocks;

  NumDriverLocks = 0;
  if (_IsInited == 0) {

#if (FS_OS == 1) && (FS_OS_LOCK_PER_DRIVER == 1)
    //
    //  Add all drivers that should be used
    //
    FS_X_AddDevices();
    //
    //  Calc the number of locks that are needed.
    //
    NumDriverLocks = FS_OS_GETNUM_DRIVERLOCKS();
    NumLocks       = FS_OS_GETNUM_SYSLOCKS() + NumDriverLocks;
    //
    //  Tell OS layer how many locks are necessary
    //
    FS_OS_INIT(NumLocks);
#else
    //
    //  Calc the number of locks that are needed.
    //
    NumDriverLocks = FS_OS_GETNUM_DRIVERLOCKS();
    NumLocks       = FS_OS_GETNUM_SYSLOCKS() + NumDriverLocks++;
    //
    //  Tell OS layer how many locks are necessary
    //
    FS_OS_INIT(NumLocks);
    FS_X_AddDevices();
    FS_USE_PARA(NumLocks);
#endif
    _IsInited |= (1 << 0);  // Set InitStatus to FS-Storage init state.
  }
  return NumDriverLocks;
}


/*********************************************************************
*
*       FS_Init
*
*  Function description:
*    Start the file system.
*
*/
void FS_Init(void) {
  unsigned NumDriverLocks;
  //
  // Allocate memory for sector buffers
  //
  NumDriverLocks = FS_STORAGE_Init();
  if (NumDriverLocks) {
    U8            * pBuffer;
    SECTOR_BUFFER * pSectorBuffer;
    unsigned        i;

    _NumSectorBuffers = FS_NUM_MEMBLOCKS_PER_OPERATION * NumDriverLocks;
    //
    // Alloc memory for the SECTOR_BUFFER structure.
    //
    _paSectorBuffer   = (SECTOR_BUFFER *)FS_AllocZeroed(_NumSectorBuffers * sizeof(SECTOR_BUFFER));
    pBuffer           = (U8 *)FS_AllocZeroed(FS__MaxSectorSize * _NumSectorBuffers);
    pSectorBuffer     = _paSectorBuffer;
    for (i = 0; i < _NumSectorBuffers; i++) {
      pSectorBuffer->pBuffer = (U32 *)pBuffer;
      pBuffer               += FS__MaxSectorSize;
      pSectorBuffer++;
    }
    _IsInited |= (1 << 1);  // Set InitStatus to FS-Complete init state.
  }
}

/*********************************************************************
*
*       FS_SetMaxSectorSize
*
*  Function description:
*    Sets the max sector size that can be used.
*
*/
void FS_SetMaxSectorSize(unsigned MaxSectorSize) {
  if (_IsInited != 3) {
    // ToDO: Check MaxSectorSize for valid value ( 512, 1024, 2048 ...)
    FS__MaxSectorSize = MaxSectorSize;
  } else {
    FS_DEBUG_WARN("FS_SetMaxSectorSize() can only be called before FS_Init() or in FS_X_AddDevices().");
  }
}
/*************************** End of file ****************************/
