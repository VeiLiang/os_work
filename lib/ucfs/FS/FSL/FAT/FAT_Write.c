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
File        : FS_FAT_Write.c
Purpose     : FAT filesystem file write routines
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "FAT_Intern.h"

/*********************************************************************
*
*       Data structures
*
**********************************************************************
*/

#if FS_SUPPORT_BURST

typedef struct {
  U32       FirstSector;
  U32       NumSectors;
  FS_SB      * pSBData;
  const void * pData;
} BURST_INFO_W;

#endif

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _WriteBurst
*
*/
#if FS_SUPPORT_BURST
static int _WriteBurst(BURST_INFO_W * pBurstInfo) {
  if (pBurstInfo->NumSectors) {
    if (FS_LB_WriteBurst(pBurstInfo->pSBData->pPart,
                        pBurstInfo->FirstSector,
                        pBurstInfo->NumSectors,
                        pBurstInfo->pData, FS_SECTOR_TYPE_DATA))
    {
      FS_DEBUG_ERROROUT("Burst write error");
      return 1;     // Write error
    }
  }
  return 0;         // No problem !
}
#endif

/*********************************************************************
*
*       _UpdateDirEntry
*/
static void _UpdateDirEntry(FS_FILE_OBJ * pFileObj, FS_SB * pSB) {
  FS_FAT_DENTRY * pDirEntry;
  U32             TimeDate;
  U32             DirSectorNo;
  U16             BytesPerSector;
  U16             SectorOff;

  BytesPerSector = pFileObj->pVolume->FSInfo.FATInfo.BytesPerSec;
  DirSectorNo    = pFileObj->Data.Fat.DirEntrySector;
  FS__SB_SetSector(pSB, DirSectorNo, FS_SB_TYPE_DIRECTORY);
  SectorOff = (pFileObj->Data.Fat.DirEntryIndex  * sizeof(FS_FAT_DENTRY)) & (BytesPerSector - 1);
  pDirEntry = (FS_FAT_DENTRY *) (pSB->pBuffer + SectorOff);

  if (FS__SB_Read(pSB) == 0) {
    //
    // Modify directory entry
    //
    FS_StoreU32LE(&pDirEntry->data[DIR_ENTRY_OFF_SIZE], pFileObj->Size);
    FS_FAT_WriteDirEntryCluster(pDirEntry, pFileObj->FirstCluster);
    TimeDate = FS_X_GetTimeDate();
    FS_StoreU16LE(&pDirEntry->data[DIR_ENTRY_OFF_WRITE_TIME], (U16)(TimeDate & 0xffff));
    FS_StoreU16LE(&pDirEntry->data[DIR_ENTRY_OFF_WRITE_DATE], (U16)(TimeDate >> 16));
    FS__SB_Flush(pSB);                   /* Write the modified directory entry */
  }
}

/*********************************************************************
*
*       _WriteData
*
*  Return value
*    Number of bytes written
*/
static U32 _WriteData(const U8 * pData, U32 NumBytes2Write, FS_FILE *pFile, FS_SB * pSBData, FS_SB * pSBfat) {
  U32           NumBytesWritten;
  U32           NumBytesCluster;
  U32           BytesPerCluster;
  int           SectorOff;
  U32           SectorNo;
  FS_FILE_OBJ * pFileObj;
  FS_FAT_INFO * pFATInfo;
  char          DirUpdateRequired;
  char          ZeroCopyAllowed;
  U32           LastByteInCluster;
#if FS_SUPPORT_BURST
  BURST_INFO_W    BurstInfo;
#endif
  //
  // Init / Compute some values used thruout the routine
  //
  DirUpdateRequired     = 0;
  pFileObj              = pFile->pFileObj;
  pFATInfo              = &pFileObj->pVolume->FSInfo.FATInfo;
  BytesPerCluster       = pFATInfo->BytesPerCluster;
  NumBytesWritten       = 0;
#if FS_SUPPORT_BURST
  BurstInfo.NumSectors  = 0;
  BurstInfo.FirstSector = 0xFFFFFFFF;
  BurstInfo.pSBData     = pSBData;
#endif
  ZeroCopyAllowed = 1;
#if FS_SUPPORT_CHECK_MEMORY
  {
    FS_MEMORY_IS_ACCESSIBLE_CALLBACK * pfMemoryIsAccessible;

    pfMemoryIsAccessible = pFileObj->pVolume->Partition.Device.Data.pfMemoryIsAccessible;
    if (pfMemoryIsAccessible) {
      if (pfMemoryIsAccessible((void *)pData, NumBytes2Write) == 0) {
        ZeroCopyAllowed = 0;
      }
    }
  }
#endif
  //
  // Main loop
  // We determine the cluster (allocate as necessary using the FAT buffer)
  // and write data into the cluster
  //
  do {
    //
    // Locate current cluster.
    //
    if (FS_FAT_GotoClusterAllocIfReq(pFile, pSBfat)) {
      FS_DEBUG_ERROROUT("Could not alloc cluster to file");
#if FS_SUPPORT_BURST
      if (_WriteBurst(&BurstInfo)) {
        NumBytesWritten = 0;               /* We do not know how many bytes have been written o.k., so reporting 0 is on the safe side */
      }
#endif
      _UpdateDirEntry(pFileObj, pSBData);
      return NumBytesWritten;           /* File truncated (too few clusters) */
    }
    LastByteInCluster = BytesPerCluster * (pFileObj->Data.Fat.CurClusterFile + 1);
    NumBytesCluster   = LastByteInCluster - pFile->FilePos;
    SectorOff         = pFile->FilePos % pFATInfo->BytesPerSec;
    if (NumBytesCluster > NumBytes2Write) {
      NumBytesCluster = NumBytes2Write;
    }
    SectorNo  = FS_FAT_ClusterId2SectorNo(pFATInfo, pFileObj->Data.Fat.CurClusterAbs);
    SectorNo += (pFile->FilePos / pFATInfo->BytesPerSec) & (pFATInfo->SecPerClus -1);
    //
    // Write data into the cluster, iterating over sectors
    //
    do {
      int NumBytesSector;
      NumBytesSector = pFATInfo->BytesPerSec - SectorOff;
      if ((U32)NumBytesSector > NumBytesCluster) {
        NumBytesSector = NumBytesCluster;
      }

      //
      // Check if we can write an entire sector
      //
      if   ((ZeroCopyAllowed == 0)
#if FS_DRIVER_ALIGMENT > 1      // Not required, just to avoid warnings
        || (((int)pData & (FS_DRIVER_ALIGMENT - 1)))
#endif
        || (NumBytesSector != pFATInfo->BytesPerSec))
      {
        //
        // Read the sector if we need to modify an existing one
        //
        if (SectorOff | (pFile->FilePos != pFileObj->Size)) {
          FS__SB_SetSector(pSBData, SectorNo, FS_SB_TYPE_DATA);
          if (FS__SB_Read(pSBData)) {
            FS_DEBUG_ERROROUT("Read error during write");
            pFile->Error = FS_ERR_READERROR;       /* read error during write */
            return NumBytesWritten;
          }
        }
        //
        // Copy the data
        //
        FS_MEMCPY(pSBData->pBuffer + SectorOff, pData, NumBytesSector);
        //
        // Write sector
        //
        FS__SB_SetSector(pSBData, SectorNo, FS_SB_TYPE_DATA);
        if (FS__SB_Write(pSBData)) {
          return NumBytesWritten;          /* Could not write data sector */
        }
      } else {
        //
        // Write the sector with "Zero-copy"
        //
        #if FS_SUPPORT_BURST == 0
          if (FS_LB_WritePart(pSBData->pPart, SectorNo, pData, FS_SECTOR_TYPE_DATA)) {
            return NumBytesWritten;
          }
        #else
          if (SectorNo != BurstInfo.FirstSector + BurstInfo.NumSectors) {
            if (_WriteBurst(&BurstInfo)) {
              return NumBytesWritten;
            }
            BurstInfo.FirstSector = SectorNo;
            BurstInfo.NumSectors  = 1;
            BurstInfo.pData       = pData;
          } else {
            BurstInfo.NumSectors++;
          }
        #endif
      }
      //
      // Update management info
      //
      pData           += NumBytesSector;
      NumBytesCluster -= NumBytesSector;
      NumBytes2Write  -= NumBytesSector;
      NumBytesWritten += NumBytesSector;
      pFile->FilePos  += NumBytesSector;
      SectorNo++;
      SectorOff = 0;                /* Next sector will be written from start */
      //
      // Update File size
      //
      if (pFile->FilePos > pFileObj->Size) {
#if FS_FAT_FWRITE_UPDATE_DIR == 0
        if (pFileObj->Size == 0)          // In this case we have allocated a cluster and need to update the dir entry!
#endif
        {
          DirUpdateRequired = 1;
        }
        pFileObj->Size = pFile->FilePos;
      }
    } while (NumBytesCluster);
  } while (NumBytes2Write);
  //
  // Flush Burst
  //
#if FS_SUPPORT_BURST
  if (_WriteBurst(&BurstInfo)) {
    NumBytesWritten = 0;               /* We do not know how many bytes have been written o.k., so reporting 0 is on the safe side */
  }
#endif
  //
  // Update directory entry if required
  //
  if (DirUpdateRequired) {
    _UpdateDirEntry(pFileObj, pSBData);
  }
  return NumBytesWritten;
}


/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       FS_FAT_GotoClusterAllocIfReq
*
*  Purpose
*    Allocates clusters to the file.
*
*  Return value
*    0     if cluster has been located
*    1     error
*/
int FS_FAT_GotoClusterAllocIfReq(FS_FILE *pFile, FS_SB * pSBfat) {
  FS_FILE_OBJ * pFileObj;
  FS_VOLUME   * pVolume;
  U32           NumClustersToGo;
  int           r;

  r               = 0;
  pFileObj        = pFile->pFileObj;
  pVolume         = pFileObj->pVolume;
  NumClustersToGo = FS_FAT_GotoCluster(pFile, pSBfat);
  if (NumClustersToGo > 0) {
    //
    //
    // Make sure at least one cluster is allocated, so that FirstCluster is valid.
    // If no cluster has yet been allocated, allocate one
    //
    if (pFileObj->FirstCluster == 0) {
      U32 CurClusterId;    /* FAT Id of the current cluster */
      CurClusterId    = FS_FAT_FindFreeCluster(pVolume, pSBfat, 0);
      if (CurClusterId == 0) {
        pFile->Error = FS_ERR_DISKFULL;
        return 1;                // No free cluster
      }
      pFileObj->FirstCluster = CurClusterId;
      NumClustersToGo--;
      pFileObj->Data.Fat.CurClusterAbs  = CurClusterId;
      pFileObj->Data.Fat.CurClusterFile = 0;
      if (FS_FAT_MarkClusterEOC(pVolume, pSBfat, pFileObj->Data.Fat.CurClusterAbs)) {
        FS_DEBUG_ERROROUT("Could not write FAT entry");
        return 1;                // Error
      }
    }
    if (NumClustersToGo) {
      do {
        U32 NewCluster;
        //
        // Check if we have an other cluster in the chain or if we need to alloc an other one
        //
        NewCluster = FS_FAT_FindFreeCluster(pVolume, pSBfat, pFileObj->Data.Fat.CurClusterAbs);
        if (NewCluster == 0) {
          pFile->Error = FS_ERR_DISKFULL;
          r =  1;                                /* Error, disk full */
          break;
        }
        FS_FAT_LinkCluster(pVolume, pSBfat, pFileObj->Data.Fat.CurClusterAbs, NewCluster);
        pFileObj->Data.Fat.CurClusterAbs = NewCluster;
        pFileObj->Data.Fat.CurClusterFile++;
      } while (--NumClustersToGo);
      //
      // Mark the last allocated cluster as the last in the chain.
      //
      if (FS_FAT_MarkClusterEOC(pVolume, pSBfat, pFileObj->Data.Fat.CurClusterAbs)) {
        FS_DEBUG_ERROROUT("Could not write FAT entry");
        return 1;                // Error
      }
    }
  }
  return r;               /* O.K. */
}


/*********************************************************************
*
*       FS_FAT_Write
*
*  Description:
*    FS internal function. Write data to a file.
*
*  Parameters:
*    pData           - Pointer to data, which will be written to the file.
*    NumBytes        - Size of an element to be transferred to a file.
*    pFile           - Pointer to a FS_FILE data structure.
*
*  Note:
*    pFile is not checked if it is valid
*
*  Return value:
*    Number of bytes written.
*/
U32 FS_FAT_Write(FS_FILE *pFile, const void *pData, U32 NumBytes) {
  U32        NumBytesWritten;
  FS_SB         sbData;          /* Sector buffer for Data */
  FS_SB         sbfat;           /* Sector buffer for FAT handling */
  FS_FILE_OBJ * pFileObj;
  FS_VOLUME   * pVolume;

  pFileObj = pFile->pFileObj;
  pVolume  = pFileObj->pVolume;
  //
  // Check if file status is O.K..
  // If not, return.
  //
  if (pFile->Error) {
    return 0;                 // Error
  }
  //
  // Allocate sector buffers.
  //
  FS__SB_Create(&sbfat,  &pVolume->Partition);
  FS__SB_Create(&sbData, &pVolume->Partition);
  //
  // Do the work in a static subroutine
  //
  NumBytesWritten = _WriteData((const U8 *)pData, NumBytes, pFile, &sbData, &sbfat);
  /*
   * If less bytes have been written than intended
   *   - Set error code in file structur (unless already set)
   *   - Invalidate the Current cluster Id to make sure we read allocation list from start next time we read
   */
  if (NumBytesWritten != NumBytes) {
    if (pFile->Error == 0) {
      pFile->Error = FS_ERR_WRITEERROR;
    }
  }
  /*
   * Cleanup
   */
  FS__SB_Delete(&sbfat);
  FS__SB_Delete(&sbData);
  return NumBytesWritten;
}

/*********************************************************************
*
*       FS_FAT_Close
*
*  Description:
*    FS internal function. Close a file referred by a file pointer.
*
*  Parameters:
*    pFile       - Pointer to a FS_FILE data structure.
*
*  Return value:
*    None.
*/
void FS_FAT_Close(FS_FILE *pFile) {
  FS_FILE_OBJ * pFileObj;
  FS_VOLUME   * pVolume;

  pFileObj = pFile->pFileObj;
  /* Check if media is OK */
  pVolume  = pFileObj->pVolume;
  if (pFile->Error == 0) {
    /*
     * Update directory if necessary
     */
    if (pFile->AccessFlags & FS_FILE_ACCESS_FLAGS_AW) {
      FS_SB     SB;          /* Sector buffer for Data */
      FS__SB_Create(&SB, &pVolume->Partition);
      _UpdateDirEntry(pFileObj, &SB);
      FS__SB_Delete(&SB);
    }
  }
}

/*********************************************************************
*
*       FS_FAT_Unmount
*
*  Description:
*    Unmounts a volume. If any pending operations need to be done to
*    the FAT FS (eg. Updating the FSInfo on FAT32 media), this is done
*    in this function.
*
*  Parameters:
*    pVolume       - Pointer to a mounted volume.
*
*  Return value:
*    None.
*/
void FS_FAT_Unmount(FS_VOLUME * pVolume) {
#if FS_FAT_USE_FSINFO_SECTOR
  FS_FAT_INFO * pFATInfo;
  pFATInfo = &pVolume->FSInfo.FATInfo;
  if (pFATInfo->FATType == FS_FAT_TYPE_FAT32) {
    /*
     * Update the FSInfo Sector on FAT32 medium.
     */
    FS_SB         SB;
    FS__SB_Create(&SB, &pVolume->Partition);
    FS__SB_SetSector(&SB, pFATInfo->FSInfoSector, FS_SB_TYPE_DATA);
    if (FS__SB_Read(&SB) == 0) {
      U8 * pBuffer;
      pBuffer = SB.pBuffer;
      FS_StoreU32LE(&pBuffer[FSINFO_OFF_FREE_CLUSTERS],     pFATInfo->NumFreeClusters);
      FS_StoreU32LE(&pBuffer[FSINFO_OFF_NEXT_FREE_CLUSTER], pFATInfo->NextFreeCluster);
      FS__SB_MarkDirty(&SB);
    }
    FS__SB_Delete(&SB);
  }
#else
  FS_USE_PARA(pVolume);
#endif
}

/*************************** End of file ****************************/
