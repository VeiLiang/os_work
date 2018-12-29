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
File        : FS_FAT_DiskInfo.c
Purpose     : FAT File System Layer for handling disk information
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*             #include Section
*
**********************************************************************
*/

#include "FAT.h"
#include "FAT_Intern.h"


/*********************************************************************
*
*       #define constants
*
**********************************************************************
*/

/*********************************************************************
*
*       Typedefs
*
**********************************************************************
*/


/*********************************************************************
*
*       Static const
*
**********************************************************************
*/


/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

extern int xm_power_check_acc_safe_or_not (void);

/*********************************************************************
*
*       FS_FAT_GetDiskInfo
*
*  Description:
*    Store information about used/unused clusters
*    in a FS_DISKFREE_T data structure.
*
*  Parameters:
*    pDiskData   - Pointer to a FS_DISKFREE_T data structure.
*
*  Return value:
*    ==0         - Information is stored in pDiskData.
*    <0          - An error has occured.
*/
int FS_FAT_GetDiskInfo(FS_VOLUME * pVolume, FS_DISK_INFO * pDiskData) {
  FS_FAT_INFO* pFATInfo;
  U32 iCluster;
  U32 LastCluster;
  U32 NumFreeClusters;
  FS_SB  sb;

  pFATInfo = &pVolume->FSInfo.FATInfo;
  if (pDiskData == NULL) {
    return -1;  /* No pointer to a FS_DISKFREE_T structure */
  }
  FS__SB_Create(&sb, &pVolume->Partition);
  LastCluster = pFATInfo->NumClusters + 1;

  if ((pFATInfo->NumFreeClusters != INVALID_NUM_FREE_CLUSTERS_VALUE) && (pFATInfo->NumFreeClusters <= pFATInfo->NumClusters)) {
    NumFreeClusters = pFATInfo->NumFreeClusters;
  } else {
    /*
     * Start to count the empty clusters
     */
    NumFreeClusters = 0;
    for (iCluster = 2; iCluster <= LastCluster; iCluster++) {
      if (FS_FAT_ReadFATEntry(pVolume, &sb, iCluster) == 0) {
        NumFreeClusters++;
      }
		// DiskInfo扫描会持续很长时间(10秒~30秒), 取决于SD卡容量及簇大小 
		// 每隔固定间隔检测ACC供电是否安全, 若不安全, 终止并退出, 避免长时间检查延迟ACC断电关机
		if(iCluster % 64 == 0)
		{
			if(!xm_power_check_acc_safe_or_not())
				break;
		}
    }
    pFATInfo->NumFreeClusters = NumFreeClusters;         /* Update FATInfo */
  }

  pDiskData->NumTotalClusters  = pFATInfo->NumClusters;
  pDiskData->NumFreeClusters   = NumFreeClusters;
  pDiskData->SectorsPerCluster = pFATInfo->SecPerClus;
  pDiskData->BytesPerSector    = pFATInfo->BytesPerSec;
  FS__SB_Delete(&sb);
  return 0;
}

/*********************************************************************
*
*       FS_FAT_GetDiskSpace
*
*  Description:
*    Return the available disk space on volume
*
*  Return value:
*    !=0          - Size of disk in bytes.
*    ==0          - An error has occured.
*    ==0xFFFFFFFF - Disk size > 4GBytes.
*/
U32 FS_FAT_GetDiskSpace(FS_VOLUME * pVolume) {
  U32 r;
  FS_FAT_INFO * pFATInfo;

  pFATInfo = &pVolume->FSInfo.FATInfo;
  r = FS__CalcSizeInBytes(pFATInfo->NumClusters, pFATInfo->SecPerClus, pFATInfo->BytesPerSec);
  return r;
}

/*************************** End of file ****************************/
