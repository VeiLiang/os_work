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
File        : FS_CacheMan.c
Purpose     : Logical Block Layer, Cache module
              Cache Strategy:
                Pure read cache, caching management sectors only.
                Which sectors are management sectors is determined
                by the File system. In case of FAT, only FAT sectors
                are considered management sectors.
              Limitations:
                None.
                This cache module can be used on any device with any
                file system.
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "FS_Int.h"

#if FS_SUPPORT_CACHE



/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#define SECTOR_INDEX_INVALID   0xFFFFFFFF

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

/* Block info. One instance per block.
   Every cache block can cache a single sector.
   It starts with CACHE_BLOCK_INFO_MAN, followed by the cached data. */
typedef struct {
  U32 SectorNo;
} CACHE_BLOCK_INFO_MAN;

/* Cache data. Once instance per cache.
   Size needs to be a multiple of 4 */
typedef struct {
  U32 NumSectors;
  U32 SectorSize;
} CACHE_DATA_MAN;

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

static U32 _GetHashCode(U32 SectorNo, U32 NumSectorIndices) {
  return SectorNo % NumSectorIndices;
}

/*********************************************************************
*
*       Static code (callbacks)
*
**********************************************************************
*/

/*********************************************************************
*
*       _CacheMan_ReadFromCache
*
*  Description:
*    Read sector from cache if possible
*
*  Return value
*    1    Sector not found
*    0    Sector found
*/
static char _CacheMan_ReadFromCache(void * p, U32 SectorNo, void * pData, U8 SectorType) {
  U32             Off;
  CACHE_DATA_MAN       * pCacheData;
  CACHE_BLOCK_INFO_MAN * pBlockInfo;
  U32             SectorSize;

  FS_USE_PARA(SectorType);
  pCacheData  = (CACHE_DATA_MAN *)p;
  SectorSize  = pCacheData->SectorSize;
  Off         = _GetHashCode(SectorNo, pCacheData->NumSectors) * (sizeof(CACHE_BLOCK_INFO_MAN) + SectorSize);
  pBlockInfo  = (CACHE_BLOCK_INFO_MAN *) (((U8 *)(pCacheData + 1)) + Off);
  if (pBlockInfo->SectorNo == SectorNo) {
    FS_MEMCPY(pData, pBlockInfo + 1, SectorSize);
    return 0;                         /* Sector found */
  }
  return 1;                         /* Sector not found */
}

/*********************************************************************
*
*       _WriteIntoCache
*
*  Description:
*    Writes a sector in cache.
*/
static void _WriteIntoCache(CACHE_BLOCK_INFO_MAN  * pBlockInfo, U32 SectorNo, const void * pData, U32 SectorSize) {
  pBlockInfo->SectorNo = SectorNo;
  FS_MEMCPY(pBlockInfo + 1, pData, SectorSize);
}

/*********************************************************************
*
*       _CacheMan_WriteCache
*
*  Description:
*    Write sector into cache
*
*  Return value
*    0    Not in write cache, the physical write operation still needs to be performed (Since this cache is a pure read-cache).
*/
static char _CacheMan_WriteCache(FS_DEVICE * pDevice, U32 SectorNo, const void * pData, U8 SectorType) {
  U32             Off;
  CACHE_DATA_MAN       * pCacheData;
  CACHE_BLOCK_INFO_MAN * pBlockInfo;
  U32             SectorSize;

  pCacheData  = (CACHE_DATA_MAN *)pDevice->Data.pCacheData;
  if (SectorType == FS_SECTOR_TYPE_MAN) {
    SectorSize  = pCacheData->SectorSize;
    Off         = _GetHashCode(SectorNo, pCacheData->NumSectors) * (sizeof(CACHE_BLOCK_INFO_MAN) + SectorSize);
    pBlockInfo  = (CACHE_BLOCK_INFO_MAN *) (((U8 *)(pCacheData + 1)) + Off);
    _WriteIntoCache(pBlockInfo, SectorNo, pData, SectorSize);
  }
  return 0;
}

/*********************************************************************
*
*       _CacheMan_InvalidateCache
*
*  Description:
*    Invalidates all data in cache
*/
static void _CacheMan_InvalidateCache (void * p) {
  U32             i;
  U32             NumSectors;
  CACHE_DATA_MAN *       pCacheData;
  CACHE_BLOCK_INFO_MAN * pBlockInfo;
  U32             SectorSize;

  pCacheData = (CACHE_DATA_MAN *)p;
  NumSectors = pCacheData->NumSectors;
  SectorSize = pCacheData->SectorSize;
  pBlockInfo = (CACHE_BLOCK_INFO_MAN *)(pCacheData + 1);
  /* Init Cache entries */
  for (i = 0; i < NumSectors; i++) {
    pBlockInfo->SectorNo = SECTOR_INDEX_INVALID;
    pBlockInfo = (CACHE_BLOCK_INFO_MAN*)(((U8*)(pBlockInfo + 1)) + SectorSize);
  }
}

/*********************************************************************
*
*       _CacheMan_Command
*
*  Description:
*    Execute commands on the cache
*
*  Return value:
*    Unsupported command:    -1
*    Supported commands:     <0: Error. Precise value depends on command
*/
static int _CacheMan_Command(FS_DEVICE * pDevice, int Cmd, void *p) {
  int r;

  r  = -1;
  switch (Cmd) {
  case FS_CMD_CACHE_INVALIDATE:
    _CacheMan_InvalidateCache(p);
    r = 0;
    break;
  }
  return r;
}

/*********************************************************************
*
*       _CacheAPI
*
*/
static const FS_CACHE_API _CacheManAPI = {
  _CacheMan_ReadFromCache,
  _CacheMan_WriteCache,
  _CacheMan_InvalidateCache,
  _CacheMan_Command,
  _CacheMan_WriteCache,
  
#if FS_PREFETCH_FAT
  0
#endif
};

/*********************************************************************
*
*       Public data
*
**********************************************************************
*/


/*********************************************************************
*
*       FS_CacheMan_Init
*
*  Description:
*    Initializes the cache
*/
U32 FS_CacheMan_Init(FS_DEVICE * pDevice, void * pData, I32 NumBytes) {
  FS_DEVICE_DATA * pDevData;
  U32           NumSectors;
  U32           SectorSize;

  NumSectors = 0;
  pDevData   = &pDevice->Data;
  SectorSize = FS_GetSectorSize(pDevice);
  /* Compute number of sectors in cache */
  NumSectors = (NumBytes - sizeof(CACHE_DATA_MAN)) / (sizeof(CACHE_BLOCK_INFO_MAN) + SectorSize);
  if (NumSectors > 0) {
    CACHE_DATA_MAN * pCacheData;

    pCacheData = (CACHE_DATA_MAN *)pData;
    FS_MEMSET(pCacheData, 0, sizeof(CACHE_DATA_MAN));
    pCacheData->NumSectors  = NumSectors;
    pCacheData->SectorSize  = SectorSize;
    pDevData->pCacheAPI     = &_CacheManAPI;
    pDevData->pCacheData    = pCacheData;
    _CacheMan_InvalidateCache(pCacheData);
  } else {
    pDevData->pCacheAPI     = NULL;
  }
  return NumSectors;
}
#else
 void CacheMan_c(void);
 void CacheMan_c(void) {}

#endif /* FS_SUPPORT_CACHE */

/*************************** End of file ****************************/
