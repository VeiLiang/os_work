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
File        : FS_CacheRW.c
Purpose     : Logical Block Layer, Cache module
              Cache Strategy:
                Read / write cache, caching all sectors equally.
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
   It starts with CACHE_BLOCK_INFO_RW, followed by the cached data. */
typedef struct {
  U32     SectorNo;
  //unsigned   IsDirty;
  U32   IsDirty;
  U8		aPadding[32-8];
} CACHE_BLOCK_INFO_RW;

/* Cache data. Once instance per cache.
   Size needs to be a multiple of 32 */
typedef struct {
  U32        NumSectors;
  U32        SectorSize;
  U8         aCacheMode[FS_SECTOR_TYPE_COUNT];
  U8         aPadding[32 - 8 - FS_SECTOR_TYPE_COUNT];     /* Make sure we pad this to a multiple of 32 bytes */
} CACHE_DATA_RW;

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _GetHashCode
*
*  Description:
*    Calculates hashcode, based on sector number and Number of sectors in cache
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
*       _CacheRW_ReadFromCache
*
*  Description:
*    Read sector from cache if possible
*
*  Return value
*    1    Sector not found
*    0    Sector found
*/
static char _CacheRW_ReadFromCache(void * p, U32 SectorNo,       void * pData, U8 SectorType) {
  U32             Off;
  CACHE_DATA_RW       * pCacheData;
  CACHE_BLOCK_INFO_RW * pBlockInfo;
  U32             SectorSize;

  FS_USE_PARA(SectorType);
  pCacheData  = (CACHE_DATA_RW *)p;
  SectorSize  = pCacheData->SectorSize;
  Off         = _GetHashCode(SectorNo, pCacheData->NumSectors) * (sizeof(CACHE_BLOCK_INFO_RW) + SectorSize);
  pBlockInfo  = (CACHE_BLOCK_INFO_RW *) (((U8 *)(pCacheData + 1)) + Off);
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
static void _WriteIntoCache(CACHE_BLOCK_INFO_RW  * pBlockInfo, U32 SectorNo, const void * pData, U32 SectorSize) {
  pBlockInfo->SectorNo = SectorNo;
  FS_MEMCPY(pBlockInfo + 1, pData, SectorSize);
}

/*********************************************************************
*
*       _CacheRW_UpdateCache
*
*  Description:
*    Updates a sector in cache.
*    Called after a READ operation to update the cache.
*    This means that the sector can not be in the cache.
*
*  Return value
*    0    Not in write cache, the physical write operation still needs to be performed (Since this cache is a pure read-cache).
*/
static char _CacheRW_UpdateCache(FS_DEVICE * pDevice, U32 SectorNo, const void * pData, U8 SectorType) {
  U32             Off;
  CACHE_DATA_RW       * pCacheData;
  CACHE_BLOCK_INFO_RW * pBlockInfo;
  U32             SectorSize;
  int                CacheMode;

  pCacheData  = (CACHE_DATA_RW *)pDevice->Data.pCacheData;
  CacheMode   = pCacheData->aCacheMode[SectorType];
  if (CacheMode & FS_CACHE_MODE_R) {           /* Read cache is on for this type of sector */
    SectorSize  = pCacheData->SectorSize;
    Off         = _GetHashCode(SectorNo, pCacheData->NumSectors) * (sizeof(CACHE_BLOCK_INFO_RW) + SectorSize);
    pBlockInfo  = (CACHE_BLOCK_INFO_RW *) (((U8 *)(pCacheData + 1)) + Off);
    /* If we replace an other, dirty sector, we need to write it out */
    if ((pBlockInfo->SectorNo != SectorNo) && (pBlockInfo->IsDirty)) {
      if ((pDevice->pType->pfWrite)(pDevice->Data.Unit, pBlockInfo->SectorNo, pBlockInfo + 1, 1, 0)) {
        FS_DEBUG_ERROROUT("Failure when cleaning cache");   /* FATAL error ! */
      }
      FS_DEBUG_LOGF((" Cleaning %s:%d: Sector: 0x%.8X", pDevice->pType->pfGetName(pDevice->Data.Unit), pDevice->Data.Unit, pBlockInfo->SectorNo));
    }
    _WriteIntoCache(pBlockInfo, SectorNo, pData, SectorSize);
    pBlockInfo->IsDirty = 0;
  }
  return 0;
}

/*********************************************************************
*
*       _CacheRW_WriteCache
*
*  Description:
*    Writes a sector into cache.
*
*  Return value
*    0    Not  in write cache, the physical write operation still needs to be performed (Since this cache is a pure read-cache).
*    1    Data in write cache, the physical write operation does not need to be performed.
*/
static char _CacheRW_WriteCache(FS_DEVICE * pDevice, U32 SectorNo, const void * pData, U8 SectorType) {
  U32             Off;
  CACHE_DATA_RW       * pCacheData;
  CACHE_BLOCK_INFO_RW * pBlockInfo;
  U32             SectorSize;
  int                CacheMode;
  char               WriteRequired;

  pCacheData    = (CACHE_DATA_RW *)pDevice->Data.pCacheData;
  CacheMode     = pCacheData->aCacheMode[SectorType];
  SectorSize    = pCacheData->SectorSize;
  Off           = _GetHashCode(SectorNo, pCacheData->NumSectors) * (sizeof(CACHE_BLOCK_INFO_RW) + SectorSize);
  pBlockInfo    = (CACHE_BLOCK_INFO_RW *) (((U8 *)(pCacheData + 1)) + Off);
  WriteRequired = 0;
  if (CacheMode & FS_CACHE_MODE_W) {              /* Write cache on for this type of sector ? */
    WriteRequired = 1;
  } else if (pBlockInfo->SectorNo == SectorNo) {  /* Sector already in cache ? */
    WriteRequired = 1;                            /* Update required ! */
  }
  if (WriteRequired) {
    /* If we replace an other, dirty sector, we need to write it out */
    if ((pBlockInfo->IsDirty) && (pBlockInfo->SectorNo != SectorNo)) {
      if ((pDevice->pType->pfWrite)(pDevice->Data.Unit, pBlockInfo->SectorNo, pBlockInfo + 1, 1, 0)) {
        FS_DEBUG_ERROROUT("Failure when cleaning cache");   /* FATAL error ! */
      }
      FS_DEBUG_LOGF((" Cleaning %s:%d: Sector: 0x%.8X", pDevice->pType->pfGetName(pDevice->Data.Unit), pDevice->Data.Unit, pBlockInfo->SectorNo));
    }
    pBlockInfo->IsDirty = 0;
    _WriteIntoCache(pBlockInfo, SectorNo, pData, SectorSize);
  }
  if (CacheMode & FS_CACHE_MODE_D) {              /* Delayed write allowed cache on for this type of sector ? */
    pBlockInfo->IsDirty = 1;
    return 1;                                     /* Write is delayed (data in cache) and does not need to be performed */
  } else {
    return 0;                                     /* Write still needs to be performed. */
  }
}

/*********************************************************************
*
*       _CacheRW_InvalidateCache
*
*  Description:
*    Invalidates all data in cache
*/
static void _CacheRW_InvalidateCache(void * p) {
  U32             i;
  U32             NumSectors;
  CACHE_DATA_RW *       pCacheData;
  CACHE_BLOCK_INFO_RW * pBlockInfo;
  U32             SectorSize;

  pCacheData = (CACHE_DATA_RW *)p;
  NumSectors = pCacheData->NumSectors;
  SectorSize = pCacheData->SectorSize;
  pBlockInfo = (CACHE_BLOCK_INFO_RW *)(pCacheData + 1);
  /* Init Cache entries */
  for (i = 0; i < NumSectors; i++) {
    pBlockInfo->SectorNo = SECTOR_INDEX_INVALID;
    pBlockInfo->IsDirty  = 0;
    pBlockInfo = (CACHE_BLOCK_INFO_RW*)(((U8*)(pBlockInfo + 1)) + SectorSize);
  }
}

/*********************************************************************
*
*       _SetMode
*
*  Description:
*    Sets the mode for the give type of sectors.
*
*  Return value:
*/
static void _SetMode(FS_DEVICE * pDevice, CACHE_MODE * pCacheMode) {
  int i;
  CACHE_DATA_RW * pCacheData;

  pCacheData = (CACHE_DATA_RW *)pDevice->Data.pCacheData;
  for (i = 0; i < FS_SECTOR_TYPE_COUNT; i++) {
    int TypeMask;
    TypeMask = 1 << i;
    if (TypeMask & pCacheMode->TypeMask) {
      pCacheData->aCacheMode[i] = (U8)pCacheMode->ModeMask;
    }
  }
}

#define FS_SUPPORT_MULTIPLE_ADDRESS
#ifdef FS_SUPPORT_MULTIPLE_ADDRESS
#include <stdlib.h>
int block_info_compare (const void *elem1, const void *elem2)
{
	if( ((CACHE_BLOCK_INFO_RW *)elem1)->SectorNo < ((CACHE_BLOCK_INFO_RW *)elem2)->SectorNo )
		return -1;
	else if( ((CACHE_BLOCK_INFO_RW *)elem1)->SectorNo > ((CACHE_BLOCK_INFO_RW *)elem2)->SectorNo )
		return 1;
	else		
		return 0;
}
#endif

//#define	MULT_BUFFER_COUNT		32
#define	MULT_BUFFER_COUNT		16		// 减小为16，避免DMA Write timeout
/*********************************************************************
*
*       _Clean
*
*  Description:
*    Writes out all dirty sectors from cache.
*
*  Return value:
*/
static int _Clean(FS_DEVICE * pDevice) {
  U32             i;
  U32             NumSectors;
  CACHE_DATA_RW *       pCacheData;
  CACHE_BLOCK_INFO_RW * pBlockInfo;
  U32             SectorSize;
  U32             SizeOfCacheBlock;
  
#ifdef FS_SUPPORT_MULTIPLE_ADDRESS
  // 将脏块按扇区排序，然后将相邻的扇区一次性写入，减少每次扇区单独写入时调用的开销
  CACHE_BLOCK_INFO_RW *pSortingBlockInfo[MULT_BUFFER_COUNT];
  U8 *pBufferAddr[MULT_BUFFER_COUNT];		// 脏扇区的数据地址
  U32 pBufferSize[MULT_BUFFER_COUNT];		
  U32 nDirtyBlockCount;
  U32 j;
#endif

  pCacheData = (CACHE_DATA_RW *)pDevice->Data.pCacheData;
  NumSectors = pCacheData->NumSectors;
  SectorSize = pCacheData->SectorSize;
  SizeOfCacheBlock = sizeof(CACHE_BLOCK_INFO_RW) + SectorSize;
  
#ifdef FS_SUPPORT_MULTIPLE_ADDRESS
  // 遍历所有cache中的脏扇区
  i = 0;
  while(i < NumSectors)
  {
	  U32 sector_start;
	  U32 sector_count;
	  nDirtyBlockCount = 0;
	  for (; i < NumSectors; i++)
	  {
		  pBlockInfo = (CACHE_BLOCK_INFO_RW *) (((U8 *)(pCacheData + 1)) + (i * SizeOfCacheBlock));
		  if(pBlockInfo->IsDirty)
		  {
			  pSortingBlockInfo[nDirtyBlockCount] = pBlockInfo;
			  nDirtyBlockCount ++;
			  
			  pBlockInfo->IsDirty = 0;
			  if(nDirtyBlockCount >= MULT_BUFFER_COUNT)
			  {
				  i ++;
				  break;
			  }
		  }
	  }
	  if(nDirtyBlockCount)
	  {
		  qsort ((void *)pSortingBlockInfo, nDirtyBlockCount, sizeof(CACHE_BLOCK_INFO_RW *), block_info_compare);
		  // 将相邻的扇区一次性写入
		  sector_count = 1;
		  sector_start = pSortingBlockInfo[0]->SectorNo;
		  pBufferAddr[0] = (U8 *)(pSortingBlockInfo[0] + 1);
		  pBufferSize[0] = 1;		// 累加扇区个数
		  for (j = 1; j < nDirtyBlockCount; j ++)
		  {
			  if(pSortingBlockInfo[j]->SectorNo == (sector_start + sector_count))
			  {
				  // 相邻
				  pBufferAddr[sector_count] = (U8 *)(pSortingBlockInfo[j] + 1);
				  pBufferSize[sector_count] = 1;
				  sector_count ++;
			  }
			  else
			  {
				  // 非相邻
				  
				  // 将已有相邻扇区一次性写入
				  if ((pDevice->pType->pfWriteMultBuffer)(pDevice->Data.Unit, sector_start, sector_count, sector_count, (const U8 **)pBufferAddr, pBufferSize)) {
					  FS_DEBUG_ERROROUT("Failure when cleaning cache");   /* FATAL error ! */
				  }
				  sector_start = pSortingBlockInfo[j]->SectorNo;
				  sector_count = 1;
				  pBufferAddr[0] = (U8 *)(pSortingBlockInfo[j] + 1);
				  pBufferSize[0] = 1;		// 累加扇区个数
			  }
		  }
		  
		  // 将余下相邻扇区一次性写入
		  if ((pDevice->pType->pfWriteMultBuffer)(pDevice->Data.Unit, sector_start, sector_count, sector_count, (const U8 **)pBufferAddr, pBufferSize)) {
			  FS_DEBUG_ERROROUT("Failure when cleaning cache");   /* FATAL error ! */
		  }	  
	  }
  } // while(i < NumSectors)
  
#else
  
  int err_count = 0;
  int skip = 0;
  for (i = 0; i < NumSectors; i++) {
    pBlockInfo = (CACHE_BLOCK_INFO_RW *) (((U8 *)(pCacheData + 1)) + (i * SizeOfCacheBlock));

    if (pBlockInfo->IsDirty) {
      FS_DEBUG_LOGF((" Cleaning %s:%d: Sector: 0x%.8X", pDevice->pType->pfGetName(pDevice->Data.Unit), pDevice->Data.Unit, pBlockInfo->SectorNo));
      if (skip == 0 && (pDevice->pType->pfWrite)(pDevice->Data.Unit, pBlockInfo->SectorNo, pBlockInfo + 1, 1, 0)) {
		  err_count ++;
        FS_DEBUG_ERROROUT("Failure when cleaning cache");   /* FATAL error ! */
		  if(err_count >= 3)
		  {
			// 连续3次写入失败, 放弃
			skip = 1;
		  }
      }
		else
		{
			err_count = 0;
		}
      pBlockInfo->IsDirty = 0;
    }
  }
  if(skip)
	  return -2;
#endif
  
  return 0;
}

/*********************************************************************
*
*       _CacheRW_Command
*
*  Description:
*    Execute commands on the cache
*
*  Return value:
*    Unsupported command:    -1
*    Supported commands:     <0: Error. Precise value depends on command
*/
static int _CacheRW_Command(FS_DEVICE * pDevice, int Cmd, void *p) {
  int r;

  r  = -1;
  switch (Cmd) {
  case FS_CMD_CACHE_CLEAN:
    r = _Clean(pDevice);
    break;
  case FS_CMD_CACHE_SET_MODE:
    _SetMode(pDevice, (CACHE_MODE *)p);
    r = 0;
    break;
  case FS_CMD_CACHE_INVALIDATE:
    _CacheRW_InvalidateCache(p);
    r = 0;
    break;
  }
  return r;
}

#if FS_PREFETCH_FAT

#define	PREFETCH_COUNT		4			// 预取的扇区数

int _CacheRW_Prefetch (FS_DEVICE * pDevice, U32 SectorNo, U32 SectorLimit, U8 SectorType)
{
  U32             Off;
  CACHE_DATA_RW       * pCacheData;
  CACHE_BLOCK_INFO_RW * pBlockInfo;
  U32             SectorSize;
  U32 				i, j;
  int             CacheMode;
  U8 *pBufferAddr[PREFETCH_COUNT];		
  U32 pBufferSize[PREFETCH_COUNT];		
  CACHE_BLOCK_INFO_RW *pCacheInfo[PREFETCH_COUNT];
  int r;
  
  // 仅FAT区
  if(SectorType != FS_SB_TYPE_MANAGEMENT)
	  return -1;

  r = -1;
  pCacheData  = (CACHE_DATA_RW *)pDevice->Data.pCacheData;
  CacheMode   = pCacheData->aCacheMode[SectorType];
  if (CacheMode & FS_CACHE_MODE_R) {           /* Read cache is on for this type of sector */
  	  SectorSize  = pCacheData->SectorSize;
	  for (i = 0; i < PREFETCH_COUNT; i++)
	  {
		  // 达到预取的边界
		  if((SectorNo + i) >= SectorLimit)
			  break;
		  
		  Off         = _GetHashCode(SectorNo + i, pCacheData->NumSectors) * (sizeof(CACHE_BLOCK_INFO_RW) + SectorSize);
		  pBlockInfo  = (CACHE_BLOCK_INFO_RW *) (((U8 *)(pCacheData + 1)) + Off);
		  
		  // 预取的扇区已存在, 终止预取
		  if (pBlockInfo->SectorNo == (SectorNo + i)) {
			  break;
		  }
		  
		  // 被替换的扇区脏, 将其写回
		  if (pBlockInfo->IsDirty)
		  {
			  // 脏块
			 if ((pDevice->pType->pfWrite)(pDevice->Data.Unit, pBlockInfo->SectorNo, pBlockInfo + 1, 1, 0)) {
			   FS_DEBUG_ERROROUT("Failure when cleaning cache");   /* FATAL error ! */
				break;
          }
			 pBlockInfo->IsDirty = 0;
		  }
		  
		  // 无效cache数据
		  pBlockInfo->SectorNo = SECTOR_INDEX_INVALID;
		  pCacheInfo[i] = pBlockInfo;
		  // cache块缓冲区地址
		  pBufferAddr[i] = (U8 *)(pBlockInfo + 1);
		  pBufferSize[i] = 1;
	  }
	  
	  // 检查需要预取的扇区数
	  if(i == 0)
		  return -1;		// 预取失败
	  
	  if ((pDevice->pType->pfReadMultBuffer)(pDevice->Data.Unit, SectorNo, i, i, (U8 **)pBufferAddr, pBufferSize)) {
		  FS_DEBUG_ERROROUT("Failure when cleaning cache");   /* FATAL error ! */
		  return -1;
	  }
	  
	  for (j = 0; j < i; j ++)
	  {
		  // 标记cache数据有效
		  pCacheInfo[j]->SectorNo = SectorNo + j;
	  }
	  
	  r = 0;
  }
  
  return r;
}
#endif

/*********************************************************************
*
*       _CacheRWAPI
*
*/
static const FS_CACHE_API _CacheRWAPI = {
  _CacheRW_ReadFromCache,
  _CacheRW_UpdateCache,
  _CacheRW_InvalidateCache,
  _CacheRW_Command,
  _CacheRW_WriteCache,
  
#if FS_PREFETCH_FAT
  _CacheRW_Prefetch
#endif
};

/*********************************************************************
*
*       _CacheRW_Init
*
*  Description:
*    Initializes the cache
*
*  Return value
*     Returns the number of cache blocks (Number of sectors that can be cached)
*/
U32 FS_CacheRW_Init(FS_DEVICE * pDevice, void * pData, I32 NumBytes) {
  FS_DEVICE_DATA * pDevData;
  U32           NumSectors;
  U32           SectorSize;

  NumSectors = 0;
  pDevData   = &pDevice->Data;
  SectorSize = FS_GetSectorSize(pDevice);
  /* Compute number of sectors in cache */
  NumSectors = (NumBytes - sizeof(CACHE_DATA_RW)) / (sizeof(CACHE_BLOCK_INFO_RW) + SectorSize);
  if (NumSectors > 0) {
    CACHE_DATA_RW * pCacheData;

    pCacheData = (CACHE_DATA_RW *)pData;
    FS_MEMSET(pCacheData, 0, sizeof(CACHE_DATA_RW));
    pCacheData->NumSectors  = NumSectors;
    pCacheData->SectorSize  = SectorSize;
    pDevData->pCacheAPI     = &_CacheRWAPI;
    pDevData->pCacheData    = pCacheData;
    _CacheRW_InvalidateCache(pCacheData);
  } else {
    pDevData->pCacheAPI     = NULL;
  }
  return NumSectors;
}

#else

void CacheRW_c(void);
void CacheRW_c(void) {}

#endif /* FS_SUPPORT_CACHE */

/*************************** End of file ****************************/
