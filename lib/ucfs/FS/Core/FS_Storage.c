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
File        : FS_Storage.c
Purpose     : Implementation of Storage API functions
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*             #include Section
*
**********************************************************************
*/

#include "FS_Int.h"

/*********************************************************************
*
*       Public code, internal API functions
*
**********************************************************************
*/
/*********************************************************************
*
*       FS__WriteSector
*
*  Function description:
*    Internal version of FS_WriteSector
*    Writes a sector to a device
*
*  Return value:
*       0             O.K.
*    != 0             Error
*/
int FS__WriteSector(FS_VOLUME * pVolume, const void *pData, U32 SectorIndex) {
  FS_DEVICE  * pDevice;
  int r;

  r = -1;
  if (pVolume) {
    pDevice = &pVolume->Partition.Device;
    FS_LOCK_DRIVER(pDevice);
    FS_LB_InitMediumIfRequired(pDevice);
    r = FS_LB_WriteDevice(pDevice, SectorIndex, pData, FS_SECTOR_TYPE_DATA);
    FS_UNLOCK_DRIVER(pDevice);
  }
  return r;
}

/*********************************************************************
*
*       FS__ReadSector
*
*  Function description:
*    Internal version of FS_ReadSector
*    Reads a sector to a device
*
*  Return value:
*       0             O.K.
*    != 0             Error
*/
int FS__ReadSector(FS_VOLUME * pVolume, void *pData, U32 SectorIndex) {
  FS_DEVICE  * pDevice;
  int r;

  r = -1;
  if (pVolume) {
    pDevice = &pVolume->Partition.Device;
    FS_LOCK_DRIVER(pDevice);
    FS_LB_InitMediumIfRequired(pDevice);
    r = FS_LB_ReadDevice(pDevice, SectorIndex, pData, FS_SECTOR_TYPE_DATA);
    FS_UNLOCK_DRIVER(pDevice);
  }
  return r;
}

/*********************************************************************
*
*       FS__WriteSectors
*
*  Function description:
*    Writes multiple sectors to a volume.
*
*  Return value:
*       0             O.K.
*    != 0             Error
*/
int FS__WriteSectors(FS_VOLUME * pVolume, const void *pData, U32 SectorIndex, U32 NumSectors) {
  FS_DEVICE  * pDevice;
  int r;

  r = -1;
  if (pVolume) {
    pDevice = &pVolume->Partition.Device;
    FS_LOCK_DRIVER(pDevice);
    FS_LB_InitMediumIfRequired(pDevice);
    r = FS_LB_WriteBurst(&pVolume->Partition, SectorIndex, NumSectors, pData, FS_SECTOR_TYPE_DATA);
    FS_UNLOCK_DRIVER(pDevice);
  }
  return r;
}

/*********************************************************************
*
*       FS__ReadSectors
*
*  Function description:
*    Reads multiple sectors from a volume.
*
*  Return value:
*       0             O.K.
*    != 0             Error
*/
int FS__ReadSectors(FS_VOLUME * pVolume, void *pData, U32 SectorIndex, U32 NumSectors) {
  FS_DEVICE  * pDevice;
  int r;

  r = -1;
  if (pVolume) {
    pDevice = &pVolume->Partition.Device;
    FS_LOCK_DRIVER(pDevice);
    FS_LB_InitMediumIfRequired(pDevice);
    r = FS_LB_ReadBurst(&pVolume->Partition, SectorIndex, NumSectors, pData, FS_SECTOR_TYPE_DATA);
    FS_UNLOCK_DRIVER(pDevice);
  }
  return r;
}

/*********************************************************************
*
*       FS__UnmountLL
*
*  Description:
*    Internal version of unmounting a volume at driver layer.
*    Sends an unmount command to the driver, marks the volume
*    as unmounted and uninitialized.
*
*  Parameters:
*    pVolume       Volume to unmount. Must be valid, may not be NULL.
*/
void FS__UnmountLL(FS_VOLUME * pVolume) {
  FS_DEVICE * pDevice;

  pDevice = &pVolume->Partition.Device;
  //
  // Check if we need to low-level-unmount
  //
  if ((pDevice->Data.IsInited) == 0 && (pVolume->IsMounted == 0)) {
    return;
  }
  FS_LOCK_DRIVER(pDevice);
  FS__IoCtlNL(pVolume, FS_CMD_UNMOUNT, 0, NULL);    // Send unmount command to driver
  FS_LOCK_SYS();
  pDevice->Data.IsInited = 0;
  FS_UNLOCK_SYS();
  FS_UNLOCK_DRIVER(pDevice);
}

/*********************************************************************
*
*       FS__UnmountForcedLL
*
*  Description:
*    Internal version of force-unmounting a volume at driver layer.
*    Sends a forced unmount command to the driver, marks the volume
*    as unmounted and uninitialized.
*
*  Parameters:
*    pVolume       Volume to unmount. Must be valid, may not be NULL.
*/
void FS__UnmountForcedLL(FS_VOLUME * pVolume) {
  FS_DEVICE * pDevice;

  pDevice = &pVolume->Partition.Device;
  //
  // Check if we need to low-level-unmount
  //
  if ((pDevice->Data.IsInited) == 0 && (pVolume->IsMounted == 0)) {
    return;
  }
  FS_LOCK_DRIVER(pDevice);
  FS__IoCtlNL(pVolume, FS_CMD_UNMOUNT_FORCED, 0, NULL);    // Send forced unmount command to driver
  FS_LOCK_SYS();
  pDevice->Data.IsInited = 0;
  FS_UNLOCK_SYS();
  FS_UNLOCK_DRIVER(pDevice);
}

/*********************************************************************
*
*       FS_STORAGE_Sync
*
*  Function description:
*    Cleans all caches related to the volume.
*    Informs the driver driver about the sync operation
*    thru IOCTL command FS_CMD_SYNC_VOLUME
*
*  Notes
*    There can be 2 types of caches related to the volume:
*      - High level (above driver)
*      - Low level (inside of driver)
*/
void FS__Sync(FS_VOLUME * pVolume) {
  if (pVolume) {
#if FS_SUPPORT_CACHE
    FS__CACHE_CommandVolume(pVolume, FS_CMD_CACHE_CLEAN, NULL);
#endif
    FS__IoCtl              (pVolume, FS_CMD_SYNC, 0, NULL);
  } else {
    FS_DEBUG_WARN("pVolume is invalid");
  }
}

/*********************************************************************
*
*       Public code, API functions
*
**********************************************************************
*/
/*********************************************************************
*
*       FS_STORAGE_WriteSector
*
*  Function description:
*    Writes a sector to a device
*
*  Return value:
*       0             O.K.
*    != 0             Error
*/
int FS_STORAGE_WriteSector(const char *sVolume, const void *pData, U32 SectorIndex) {
  int r;
  FS_VOLUME  * pVolume;

  FS_LOCK();
  pVolume = FS__FindVolume(sVolume, NULL);
  r = FS__WriteSector(pVolume, pData, SectorIndex);
  FS_UNLOCK();
  return r;
}

/*********************************************************************
*
*       FS_STORAGE_ReadSector
*
*  Function description:
*    Reads a sector from a device
*
*  Return value:
*       0             O.K.
*    != 0             Error
*/
int FS_STORAGE_ReadSector(const char * sVolume, void *pData, U32 SectorIndex) {
  int r;
  FS_VOLUME  * pVolume;

  FS_LOCK();
  pVolume = FS__FindVolume(sVolume, NULL);
  r = FS__ReadSector(pVolume, pData, SectorIndex);
  FS_UNLOCK();
  return r;
}

/*********************************************************************
*
*       FS_STORAGE_WriteSectors
*
*  Function description:
*    Writes a sector to a device
*
*  Return value:
*       0             O.K.
*    != 0             Error
*/
int FS_STORAGE_WriteSectors(const char *sVolume, const void * pData, U32 FirstSector, U32 NumSectors) {
  int r;
  FS_VOLUME  * pVolume;

  FS_LOCK();
  pVolume = FS__FindVolume(sVolume, NULL);
  r = FS__WriteSectors(pVolume, pData, FirstSector, NumSectors);
  FS_UNLOCK();
  return r;
}

/*********************************************************************
*
*       FS_STORAGE_ReadSectors
*
*  Function description:
*    Reads a sector from a device
*
*  Return value:
*       0             O.K.
*    != 0             Error
*/
int FS_STORAGE_ReadSectors(const char *sVolume, void * pData, U32 FirstSector, U32 NumSectors) {
  int r;
  FS_VOLUME  * pVolume;

  FS_LOCK();
  pVolume = FS__FindVolume(sVolume, NULL);
  r = FS__ReadSectors(pVolume, pData, FirstSector, NumSectors);
  FS_UNLOCK();
  return r;
}

/*********************************************************************
*
*       FS_STORAGE_Unmount
*
*  Description:
*    Unmountis a given volume at driver layer.
*    Sends an unmount command to the driver, marks the volume as unmounted
*    and uninitialized.
*
*  Parameters:
*    sVolume            - The volume name
*
*/
void FS_STORAGE_Unmount(const char * sVolume) {
  FS_VOLUME * pVolume;

  FS_LOCK();
  pVolume = FS__FindVolume(sVolume, NULL);
  if (pVolume) {
    FS__UnmountLL(pVolume);
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
void FS_STORAGE_UnmountForced(const char * sVolume) {
  FS_VOLUME * pVolume;

  FS_LOCK();
  pVolume = FS__FindVolume(sVolume, NULL);
  if (pVolume) {
    FS__UnmountForcedLL(pVolume);
  }
  FS_UNLOCK();
}


/*********************************************************************
*
*       FS_STORAGE_Sync
*
*  Function description:
*    Cleans all caches related to the volume.
*    Informs the driver driver about the sync operation
*    thru IOCTL command FS_CMD_SYNC_VOLUME
*
*  Notes
*    There can be 2 types of caches related to the volume:
*      - High level (above driver)
*      - Low level (inside of driver)
*/
void FS_STORAGE_Sync(const char * sVolume) {
  FS_VOLUME * pVolume;

  FS_LOCK();
  pVolume = FS__FindVolume(sVolume, NULL);
  FS__Sync(pVolume);
  FS_UNLOCK();
}

/*********************************************************************
*
*       FS_STORAGE_GetDeviceInfo
*
*  Function description:
*    Retrieves device information of a volume.
*
*  Return value:
*      0     - O.K.
*     -1     - Device is not ready or general error.
*/
int FS_STORAGE_GetDeviceInfo(const char * sVolume, FS_DEV_INFO * pDevInfo) {
  FS_VOLUME * pVolume;
  int         r;

  FS_LOCK();
  pVolume = FS__FindVolume(sVolume, NULL);
  r = FS__GetDeviceInfo(pVolume, pDevInfo);
  FS_UNLOCK();
  return r;
}


/*************************** End of file ****************************/
