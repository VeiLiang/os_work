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
File        : FS_Unmount.c
Purpose     : Implementation of FS_Unmount..
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "FS_Int.h"
//#include "DmpMemory.h"

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       FS__Unmount
*
*  Description:
*    Internal version of unmounting a device
*    Closes all open file and directory handles to the volume, marks the volume as unmounted
*
*  Parameters:
*    pVolume       Volume to unmount. Must be valid, may not be NULL.
*/
void FS__Unmount(FS_VOLUME * pVolume) {
	unsigned i;

	if (pVolume->IsMounted) {
		FS_DEVICE * pDevice;

		pDevice = &pVolume->Partition.Device;
		FS_LOCK_DRIVER(pDevice);
		FS_JOURNAL_INVALIDATE(pVolume);              // Note: If a transaction on the journal is running, data in journal is purposly discarded!
		//
		// Close all open files on this volume
		//
		for (i =0; i < COUNTOF(FS__aFilehandle); i++) {
			FS_FILE *     pFile;
			FS_FILE_OBJ * pFileObj;
			char FileIsOnThisVolume;

			//
			// Check if file is on this volume. SYS-Lock is required when going thru the data structures.
			//
			FileIsOnThisVolume = 0;
			FS_LOCK_SYS();
			pFile    = &FS__aFilehandle[i];
			if (pFile->InUse) {
				pFileObj = pFile->pFileObj;
				if (pFileObj->pVolume == pVolume) {
					FileIsOnThisVolume = 1;
				}
			}
			FS_UNLOCK_SYS();
			//
			// Close file if it is on this volume
			//
			if (FileIsOnThisVolume) {
				FS__FCloseNL(pFile);
			}
		}
  #if FS_SUPPORT_CACHE
		//
		// Clean cache
		//
		FS__CACHE_CommandDeviceNL(pDevice, FS_CMD_CACHE_CLEAN,  NULL);
		//
		// Invalidate cache
		//
		FS__CACHE_CommandDeviceNL(pDevice, FS_CMD_CACHE_INVALIDATE,  pVolume->Partition.Device.Data.pCacheData);
  #endif
		FS__IoCtlNL(pVolume, FS_CMD_UNMOUNT, 0, NULL);           // Send unmount command to driver

		FS_LOCK_SYS();
		if(pVolume->SupportSpecialCache)
		{
		#if   (FS_SUPPORT_FAT) && (! FS_SUPPORT_EFS) && (FS_FAT_TABLE_CACHE)
			FS_FAT_INFO        *  pFATInfo;

			pFATInfo      = &pVolume->FSInfo.FATInfo;
			if(pFATInfo->FatTableCache)
			{
				Dmpfree(pFATInfo->FatTableCache);
				pFATInfo->FatTableCache = NULL;
			}
		#endif

#if FS_FAT_USE_CLUSTER_CACHE
			{
				extern void FS_FAT_ClusterCacheDeInitialize (FS_FAT_INFO * pFATInfo);
				FS_FAT_INFO        *  pFATInfo;

				pFATInfo      = &pVolume->FSInfo.FATInfo;				
				FS_FAT_ClusterCacheDeInitialize (pFATInfo);
			}
#endif
			pVolume->SupportSpecialCache = 0;
		}
		pVolume->IsMounted = 0;                                  // Mark volume as unmounted
		pDevice->Data.IsInited = 0;
		FS_UNLOCK_SYS();
		FS_UNLOCK_DRIVER(pDevice);
	}
}



/*********************************************************************
*
*       FS__UnmountForcedNL
*
*  Description:
*    Umounts a devices.
*    Invalidates all open file and directory handles to the volume, marks the volume as unmounted
*    Does not perform locking.
*
*  Parameters:
*    pVolume       Volume to unmount. Must be valid, may not be NULL.
*
*/
void FS__UnmountForcedNL(FS_VOLUME * pVolume) {
  unsigned i;

  if (pVolume->IsMounted) {
    FS_DEVICE * pDevice;

    pDevice = &pVolume->Partition.Device;
    FS_JOURNAL_INVALIDATE(pVolume);
    //
    // Invalidate all open handles on this volume
    //
    FS_LOCK_SYS();
    for (i =0; i < COUNTOF(FS__aFilehandle); i++) {
      FS_FILE *     pFile;
      FS_FILE_OBJ * pFileObj;

      //
      // Check if file is on this volume. SYS-Lock is required when going thru the data structures.
      //
      pFile    = &FS__aFilehandle[i];
      if (pFile->InUse) {
        pFileObj = pFile->pFileObj;
        if (pFileObj) {
          if (pFileObj->pVolume == pVolume) {
            pFile->InUse    = 0;
            pFile->pFileObj = (FS_FILE_OBJ*)NULL;
            if (pFileObj->UseCnt) {
              pFileObj->UseCnt--;   // Could also be cleared to 0
            }
          }
        }
      }
    }
    FS_UNLOCK_SYS();
  #if FS_SUPPORT_CACHE
    //
    // Invalidate cache
    //
    FS__CACHE_CommandDeviceNL(pDevice, FS_CMD_CACHE_INVALIDATE,  pVolume->Partition.Device.Data.pCacheData);
  #endif
    FS__IoCtlNL(pVolume, FS_CMD_UNMOUNT, 0, NULL);           // Send unmount command to driver
    FS_LOCK_SYS();
	 
#if FS_FAT_USE_CLUSTER_CACHE
			{
				extern void FS_FAT_ClusterCacheDeInitialize (FS_FAT_INFO * pFATInfo);
				FS_FAT_INFO        *  pFATInfo;

				pFATInfo      = &pVolume->FSInfo.FATInfo;				
				FS_FAT_ClusterCacheDeInitialize (pFATInfo);
			}
#endif
	 
    pVolume->IsMounted = 0;                                  // Mark volume as unmounted
    pDevice->Data.IsInited = 0;
    FS_UNLOCK_SYS();
  }
  FS_JOURNAL_INVALIDATE(pVolume);
}

/*********************************************************************
*
*       FS__UnmountForced
*
*  Description:
*    Umounts a devices.
*    Invalidates all open file and directory handles to the volume, marks the volume as unmounted
*
*  Parameters:
*    pVolume       Volume to unmount. Must be valid, may not be NULL.
*
*/
void FS__UnmountForced(FS_VOLUME * pVolume) {
  FS_LOCK_DRIVER(&pVolume->Partition.Device);
  FS__UnmountForcedNL(pVolume);
  FS_UNLOCK_DRIVER(&pVolume->Partition.Device);
}
/*********************************************************************
*
*       FS_Unmount
*
*  Description:
*    Unmounts a device
*    Closes all open file and directory handles to the volume, marks the volume as unmounted
*
*  Parameters:
*    sVolume            - The volume name.
*/
void FS_Unmount(const char * sVolume) {
  FS_VOLUME * pVolume;

  FS_LOCK();
  pVolume = FS__FindVolume(sVolume, NULL);
  if (pVolume) {
    FS__Unmount(pVolume);
  }
  FS_UNLOCK();
}

/*********************************************************************
*
*       FS_UnmountForced
*
*  Description:
*    Unmounts a device
*    Closes all open file and directory handles to the volume, marks the volume as unmounted
*
*  Parameters:
*    sVolume            - The volume name.
*/
void FS_UnmountForced(const char * sVolume) {
  FS_VOLUME * pVolume;

  FS_LOCK();
  pVolume = FS__FindVolume(sVolume, NULL);
  if (pVolume) {
    FS__UnmountForced(pVolume);
  }
  FS_UNLOCK();
}

// 20180404 zhuoyonghong
// ¾í¹Ø±ÕÇ°Ö´ÐÐ
void FS_VolumnClean (const char * sVolume) {
  FS_VOLUME * pVolume;

  FS_LOCK();
  pVolume = FS__FindVolume(sVolume, NULL);
  if (pVolume) {
    FS_FAT_Unmount(pVolume);
  }
  FS_UNLOCK();
	
}

/*************************** End of file ****************************/
