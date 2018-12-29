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
File        : FS_Volume.c
Purpose     : API functions for handling Volumes
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
*       Static data
*
**********************************************************************
*/
static int         _NumVolumes;
FS_VOLUME          FS__aVolume[FS_NUM_VOLUMES];            /* Is initialized by FS_Init() */

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       Public code, internal
*
**********************************************************************
*/


/*********************************************************************
*
*       FS__GetVolumeStatus
*
*  Function description:
*    Internal version of FS_GetVolumeStatus
*    Returns the status of a volume.
*
*  Parameters
*    pVolume           - Pointer to a FS_VOLUME structure.
*                        Can be NULL
*
*  Return value:
*     FS_MEDIA_NOT_PRESENT     - Volume is not present.
*     FS_MEDIA_IS_PRESENT      - Volume is present.
*     FS_MEDIA_STATE_UNKNOWN   - Volume state is unknown.
*
*/
int FS__GetVolumeStatus(FS_VOLUME * pVolume) {
  int r;

  r = FS_MEDIA_STATE_UNKNOWN;
  if (pVolume) {
    FS_DEVICE * pDevice;

    pDevice = &pVolume->Partition.Device;
    FS_LOCK_DRIVER(pDevice);
    r = pDevice->pType->pfGetStatus(pDevice->Data.Unit);
    FS_UNLOCK_DRIVER(pDevice);
  }
  return r;
}

/*********************************************************************
*
*       FS__GetDeviceInfo
*
*  Function description:
*    Internal version of FS_GetDeviceInfo
*    Retrieves device information of a volume.
*
*  Return value:
*      0     - O.K.
*     -1     - Device is not ready or general error.
*/
int FS__GetDeviceInfo(FS_VOLUME * pVolume, FS_DEV_INFO * pDevInfo) {
  int         r;

  r =  -1;  // Set as error so far
  if (pVolume) {
    FS_DEVICE * pDevice;

    pDevice = &pVolume->Partition.Device;
    FS_LOCK_DRIVER(pDevice);
    FS_MEMSET(pDevInfo, 0, sizeof(FS_DEV_INFO));
    FS_LB_InitMediumIfRequired(pDevice);
    r = FS_LB_GetDeviceInfo(pDevice, pDevInfo);
    FS_UNLOCK_DRIVER(pDevice);
  }
  return r;
}


/*********************************************************************
*
*       FS__AddPhysDevice
*
*  Description:
*    Adds a device driver to the file system.
*
*  Parameters:
*
*  Return value:
*     >= 0    Unit no of the device.
*     <= 0    Error.
*/
int FS__AddPhysDevice(const FS_DEVICE_TYPE * pDevType) {
  int Unit;

  Unit = -1;
  if (pDevType->pfAddDevice) {
    Unit = pDevType->pfAddDevice();
    if (Unit < 0) {
      FS_DEBUG_ERROROUT("FS__AddPhysDevice: Could not add device.");
    } else {
      FS_OS_ADD_DRIVER(pDevType);
    }
  }
  return Unit;
}

/*********************************************************************
*
*       FS__AddDevice
*
*  Description:
*    Internal version of FS_AddDevice.
*    Adds a device driver to the file system.
*
*  Parameters:
*
*  Return value:
*/
FS_VOLUME * FS__AddDevice(const FS_DEVICE_TYPE * pDevType) {
  FS_VOLUME * pVolume;
  int Unit;

  pVolume = (FS_VOLUME *)NULL;
  if (_NumVolumes < FS_NUM_VOLUMES) {
    Unit = FS__AddPhysDevice(pDevType);
    if (Unit >= 0) {
      FS_DEVICE * pDevice;
	  	
      pVolume = &FS__aVolume[_NumVolumes++];
      pDevice = &pVolume->Partition.Device;
      pDevice->pType     = pDevType;
      pDevice->Data.Unit = (U8)Unit;
      pVolume->InUse     = 1;
      pVolume->AllowAutoMount = FS_MOUNT_RW;
    } else {
      FS_DEBUG_ERROROUT("FS__AddDevice: FS__AddPhysDevice failed.");
    }
  } else {
    FS_DEBUG_ERROROUT("Add. driver could not be added.");
  }
  return pVolume;
}

/*********************************************************************
*
*       FS__RemoveDevice
*
*  Description:
*    Removes a volume from the file system.
*
*  Parameters:
*    pVolume    - Pointer to a volume that should be removed.
*  Return value:
*/
void FS__RemoveDevice(FS_VOLUME * pVolume) {
  unsigned i;

  for (i = 0; i < COUNTOF(FS__aVolume); i++) {
    if (pVolume == &FS__aVolume[i]) {
      _NumVolumes--;
      pVolume->InUse = 0;
    }
  }
}

/*********************************************************************
*
*       FS__IsLLFormatted
*
*  Function description:
*    Internal version of FS_IsLLFormatted
*    Returns if a volume is low-level formatted or not.
*
*  Return value:
*      1     - Volume is low-level formatted.
*      0     - Volume is not low-level formatted.
*     -1     - Low level format not supported by volume.
*/
int FS__IsLLFormatted(FS_VOLUME * pVolume) {
  int r;

  r = FS__IoCtl(pVolume, FS_CMD_REQUIRES_FORMAT, 0, NULL);
  if (r == 0) {
    r = 1;
  } else if (r == 1) {
    r = 0;
  }
  return r;
}

/*********************************************************************
*
*       FS__FindVolume
*
*  Description:
*    Finds a volume based on the fully qualified filename.
*
*  Parameters:
*    pFullName   - Fully qualified name.
*    pFilename   - Address of a pointer, which is modified to point to
*                  the file name part of pFullName.
*
*  Return value:
*    NULL        - No matching volume found
*    else        - pointer to volume containing the file
*
*  Add. info:
*    pFullname can be as follows:
*    - "filename.ext"           e.g. "file.txt"
*    - "dev:filename.ext"       e.g. "mmc:file.txt"
*    - "dev:unit:filename.ext"  e.g. "mmc:0:file.txt"
*/
FS_VOLUME * FS__FindVolume(const char *pFullName, const char * * ppFileName) {
  const char * s;
  FS_VOLUME  * pVolume;
  int          i;
  unsigned     m;

  pVolume = &FS__aVolume[0];
  /* Find correct FSL (device:unit:name) */
  s = FS__strchr(pFullName, ':');
  
  if (s != NULL) {  
    /* Scan for device name */
    m = (int)((U32)(s) - (U32)(pFullName));      /* Compute length of specified device name */
    i = 0;
    do {
      const FS_DEVICE_TYPE * pDevice;
      FS_DEVICE_DATA       * pDevData;
      const char * sVolName;

      pDevice  = pVolume->Partition.Device.pType;
      pDevData = &pVolume->Partition.Device.Data;
      if (i >= _NumVolumes) {  
        pVolume = (FS_VOLUME *)NULL;
        break;                                         /* No matching device found */
      }
      sVolName = pDevice->pfGetName(pDevData->Unit);
      if (FS_STRLEN(sVolName) == m) {
        if (FS_STRNCMP(sVolName, pFullName, m) == 0) {
          break;
        }
      }
      pVolume++;
      i++;
    } while (1);
    s++;
    if (pVolume && *s != '\0') {
      /* Find the correct unit */
      if (*(s+1) == ':') {
        unsigned Unit;
        Unit = *s - '0';
        if (Unit < (unsigned)pVolume->Partition.Device.pType->pfGetNumUnits()) {
          pVolume += Unit;
          s += 2;
        } else {  
          pVolume = (FS_VOLUME *)NULL;
        }
      }
    }
  } else {
    /* use 1st FSL as default */
    s = pFullName;
  }
  if (ppFileName) {
    *ppFileName = s;
  }

  return pVolume;
}

/*********************************************************************
*
*       FS_GetNumVolumes
*
*  Description:
*    Returns the number of available volumes.
*/
int FS__GetNumVolumes(void) {
  return _NumVolumes;
}


/*********************************************************************
*
*       FS__GetVolumeInfoEx
*
*  Description:
*    Internal function. Get volume information
*
*  Parameters:
*    sVolume            - The volume name
*    pInfo              - A pointer to FS_DISK_INFO. Volume information
*                         will be filled in
*
*  Return value:
*    0                  - OK
*   -1                  - Error. Failed to get volume information
*/
int FS__GetVolumeInfoEx(FS_VOLUME  * pVolume, FS_DISK_INFO * pInfo) {
  int r;

  r = -1;
  if ((FS__AutoMount(pVolume) & FS_MOUNT_R) == FS_MOUNT_R) {
    FS_LOCK_DRIVER(&pVolume->Partition.Device);
    r = FS_GET_DISKINFO(pVolume, pInfo);
    FS_UNLOCK_DRIVER(&pVolume->Partition.Device);
    if (r != -1) {
      r = 0;      /* OK - volume info retrieved successfully */
    }
  }
  return r;
}

/*********************************************************************
*
*       FS__GetVolumeInfo
*
*  Description:
*    Internal function. Get volume information
*
*  Parameters:
*    sVolume            - The volume name
*    pInfo              - A pointer to FS_DISK_INFO. Volume information
*                         will be filled in
*
*  Return value:
*    0                  - OK
*   -1                  - Error. Failed to get volume information
*/
int FS__GetVolumeInfo(const char * sVolume, FS_DISK_INFO * pInfo) {
  FS_VOLUME*  pVolume;
  int r;

  r = -1;
  if (sVolume == NULL || pInfo == NULL) {
    return -1;   /* Error */
  }
  pVolume = FS__FindVolume(sVolume, NULL);
  if (pVolume) {
    r = FS__GetVolumeInfoEx(pVolume, pInfo);
  }
  return r;
}


/*********************************************************************
*
*       _Mount
*
*  Description:
*    If volume is not yet mounted, try to mount it.
*
*  Parameters:
*    pVolume       Volume to mount. Must be valid, may not be NULL.
*
*  Return value:
*       0         - Volume is mounted
*    != 0         - Volume is not mounted
*/
static int _Mount(FS_VOLUME * pVolume, U8 MountType) {
  int Status;
  FS_DEVICE * pDevice;

  pDevice = &pVolume->Partition.Device;
  //
  //  Check if the media is accessible.
  //
  Status = FS_LB_GetStatus(pDevice);
  
  if (Status >= 0) {
    FS_LB_InitMediumIfRequired(pDevice);
    //
    // Check first if there is a partition on the volume.
    //
    FS__LocatePartition(pVolume);
    //
    //  Mount the file system
    //
    if (FS_CHECK_INFOSECTOR(pVolume)) {
      pVolume->IsMounted = MountType;
#if FS_SUPPORT_MULTIPLE_FS
      #if FS_SUPPORT_FAT
        pVolume->pFS_API   = &FS_FAT_API;
      #else
        pVolume->pFS_API   = &FS_EFS_API;
      #endif
#endif
    }
  } else {
    FS_DEBUG_ERROROUT("Error: _Mount could not mount volume");
    return -1;
  }
  //
  // Mount the jounrnal if necessary.
  //
  FS_JOURNAL_MOUNT(pVolume);
  return pVolume->IsMounted;
}

/*********************************************************************
*
*       FS__MountNL
*
*  Description:
*    FS internal function.
*    If volume is not yet mounted, try to mount it.
*
*  Parameters:
*    pVolume       Volume to mount. Must be valid, may not be NULL.
*
*  Return value:
*    == 0         - Volume is not mounted.
*    == 1         - Volume is mounted read only.
*    == 2         - Volume is mounted read/write.
*    == -1        - Volume can not be mounted.
*/
int FS__MountNL(FS_VOLUME * pVolume, U8 MountType) {
  if (pVolume->IsMounted == 0) {
    //
    //  Shall we auto mount?
    //
    if (MountType != 0) {
      if (_Mount(pVolume, MountType) <= 0) {
        return -1;
      }
    }
  }
  return pVolume->IsMounted;
}

/*********************************************************************
*
*       FS__Mount
*
*  Description:
*    FS internal function.
*    If volume is not yet mounted, try to mount it.
*
*  Parameters:
*    pVolume       Volume to mount. Must be valid, may not be NULL.
*
*  Return value:
*    == 0         - Volume is not mounted.
*    == 1         - Volume is mounted read only.
*    == 2         - Volume is mounted read/write.
*    == -1        - Volume can not be mounted.
*/
int FS__Mount(FS_VOLUME * pVolume, U8 MountType) {
  int r;
  FS_DEVICE * pDevice;

  FS_LOCK_SYS();
  pDevice = &pVolume->Partition.Device;
  FS_UNLOCK_SYS();
  FS_USE_PARA(pDevice);
  FS_LOCK_DRIVER(pDevice);
  r = FS__MountNL(pVolume, MountType);
  FS_UNLOCK_DRIVER(pDevice);
  return r;
}


/*********************************************************************
*
*       FS__AutoMount
*
*  Description:
*    If volume is not yet mounted, try to mount it if allowed.
*
*  Parameters:
*    pVolume       Volume to mount. Must be valid, may not be NULL.
*
*  Return value:
*    == 0         - Volume is not mounted.
*    == 1         - Volume is mounted read only.
*    == 2         - Volume is mounted read/write.
*    == -1        - Volume can not be mounted.
*/
int FS__AutoMount(FS_VOLUME * pVolume) {
  int r;
  FS_DEVICE * pDevice;

  r = pVolume->IsMounted;
  if (r) {
    return r;
  }
  if (pVolume->AllowAutoMount == 0) {
    return 0;
  }
  //
  // Not yet mounted, automount allowed. Let's try to mount.
  //
  pDevice = &pVolume->Partition.Device;
  FS_USE_PARA(pDevice);
  FS_LOCK_DRIVER(pDevice);
  r = _Mount(pVolume, pVolume->AllowAutoMount);
  FS_UNLOCK_DRIVER(pDevice);
  return r;
}

/*********************************************************************
*
*       FS__AutoMountdNL
*
*  Description:
*    If volume is not yet mounted, try to mount it if allowed.
*    This function does not lock.
*
*  Parameters:
*    pVolume       Volume to mount. Must be valid, may not be NULL.
*
*  Return value:
*    == 0         - Volume is not mounted.
*    == 1         - Volume is mounted read only.
*    == 2         - Volume is mounted read/write.
*    == -1        - Volume can not be mounted.
*/
int FS__AutoMountNL(FS_VOLUME * pVolume) {
  int r;

  r = pVolume->IsMounted;
  if (r) {
    return r;
  }
  if (pVolume->AllowAutoMount == 0) {
    return 0;
  }
  //
  // Not yet mounted, automount allowed. Let's try to mount.
  //
  r = _Mount(pVolume, pVolume->AllowAutoMount);
  return r;
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       FS_AddDevice
*
*  Description:
*    Adds a device driver to the file system.
*
*  Parameters:
*
*  Return value:
*/
FS_VOLUME * FS_AddDevice(const FS_DEVICE_TYPE * pDevType) {
  FS_VOLUME * pVolume;
  FS_LOCK();
  pVolume = FS__AddDevice(pDevType);
  FS_UNLOCK();
  return pVolume;
}

/*********************************************************************
*
*       FS_AddDevice
*
*  Description:
*    Adds a device driver to the file system.
*
*  Parameters:
*
*  Return value:
*/
int FS_AddPhysDevice(const FS_DEVICE_TYPE * pDevType) {
  int r;
  FS_LOCK();
  r = FS__AddPhysDevice(pDevType);
  FS_UNLOCK();
  return r;
}

/*********************************************************************
*
*       FS_GetNumVolumes
*
*  Description:
*    Returns the number of available volumes.
*/
int FS_GetNumVolumes(void) {
  int r;
  FS_LOCK();
  r =  _NumVolumes;
  FS_UNLOCK();
  return r;
}


/*********************************************************************
*
*       FS_IsVolumeMounted
*
*  Description:
*    Returns if a volume is mounted and correctly formatted.
*
*  Return value:
*       1         - Volume is mounted
*       0         - Volume is not mounted or does not exist
*/
int FS_IsVolumeMounted(const char * sVolumeName) {
  int r;
  FS_VOLUME * pVolume;
  FS_LOCK();
  r = 0;
  pVolume = FS__FindVolume(sVolumeName, NULL);
  if (pVolume) {
    r = pVolume->IsMounted;
  }
  FS_UNLOCK();
  return r;
}

/*********************************************************************
*
*       FS_GetVolumeName
*
*  Description:
*    Returns the name of a volume
*/
int FS_GetVolumeName(int Index, char * pBuffer, int MaxSize) {
  FS_VOLUME            * pVolume;
  const FS_DEVICE_TYPE * pType;
  FS_DEVICE_DATA       * pDevData;
  const char           * pDevName;
  int                    r;

  FS_LOCK();
  pVolume  = &FS__aVolume[Index];
  pType    = pVolume->Partition.Device.pType;
  pDevData = &pVolume->Partition.Device.Data;
  pDevName = pType->pfGetName(pDevData->Unit);
  r        = 0;
  if (pBuffer)  {
    int LenReq;
    LenReq = FS_STRLEN(pDevName) + 5;
    if ((LenReq) > MaxSize) {
      FS_UNLOCK();
      return LenReq;
    }
    /*
     * Copy the device name
     */
    do {
      *pBuffer++ = *pDevName++;
      r++;
    } while (*pDevName);
    /*
     * Add ':'
     */
    *pBuffer++ = ':';
    /*
     * Add Unit number
     */
    *pBuffer++ = (U8) ('0' + pDevData->Unit);
    /*
     * Add ':'
     */
    *pBuffer++ = ':';
    r += 3;
    /*
     * Add '\0'
     */
    *pBuffer = 0;
  }
  FS_UNLOCK();
  return r;
}

/*********************************************************************
*
*       FS_GetVolumeInfo
*
*  Description:
*    Get volume information
*
*  Parameters:
*    sVolume            - The volume name
*    pInfo              - A pointer to FS_DISK_INFO. Volume information
*                         will be filled in
*
*  Return value:
*    0                  - OK
*   -1                  - Error. Failed to get volume information
*/
int FS_GetVolumeInfo(const char * sVolume, FS_DISK_INFO * pInfo) {
  int r;

  FS_LOCK();
  r = FS__GetVolumeInfo(sVolume, pInfo);
  FS_UNLOCK();
  return r;
}

/*********************************************************************
*
*       FS_GetVolumeFreeSpace
*
*  Description:
*    Returns a volume's free space in bytes
*
*  Parameters:
*    sVolume            - The volume name
*
*  Return value:
*    Number of bytes available on the volume.
*    If the volume can not be found, 0 is returned.
*
*  Notes
*    (1) Max. value:
*        Since the return value is a 32 bit value, the maximum that can
*        be return is 0xFFFFFFFF = 2^32 - 1.
*        If there is more space available than 0xFFFFFFFF, the return value is 0xFFFFFFFF.
*/
U32 FS_GetVolumeFreeSpace(const char * sVolume) {
  FS_DISK_INFO Info;
  U32       r;

  FS_LOCK();
  r = 0;
  if (sVolume) {
    if (FS__GetVolumeInfo(sVolume, &Info) != -1) {
      r = FS__CalcSizeInBytes(Info.NumFreeClusters, Info.SectorsPerCluster, Info.BytesPerSector);
    }
  }
  FS_UNLOCK();
  return r;
}

/*********************************************************************
*
*       FS_GetVolumeSize
*
*  Description:
*    Returns a volume's total size in bytes
*
*  Parameters:
*    sVolume            - The volume name
*
*  Return value:
*    The number of total bytes available on this volume, or
*    0 if the volume could not be found.
*
*  Notes
*    (1) Max. value:
*        Since the return value is a 32 bit value, the maximum that can
*        be return is 0xFFFFFFFF = 2^32 - 1.
*        If there is more space available than 0xFFFFFFFF, the return value is 0xFFFFFFFF.
*/
U32 FS_GetVolumeSize(const char * sVolume) {
  FS_DISK_INFO Info;
  U32       r;

  r = 0;          /* Error - Failed to get volume information */
  FS_LOCK();
  if (sVolume) {
    if (FS__GetVolumeInfo(sVolume, &Info) != -1) {
      r = FS__CalcSizeInBytes(Info.NumTotalClusters, Info.SectorsPerCluster, Info.BytesPerSector);
    }
  }
  FS_UNLOCK();
  return r;
}

/*********************************************************************
*
*       FS_GetVolumeLabel
*
*  Description:
*    Returns a volume label name if one exists.
*
*  Parameters:
*    sVolume            - The volume name
*    pVolumeLabel       - Pointer to a buffer to receive the volume label.
*    VolumeLabelSize    - length of pVolumeName
*
*  Return value:
*    The number of total bytes available on this volume, or
*    0 if the volume could not be found.
*/
int FS_GetVolumeLabel(const char * sVolume, char * pVolumeLabel, unsigned VolumeLabelSize) {
  int  r;
  FS_VOLUME * pVolume;
  r = -1;          /* Error - Failed to get volume information */
  FS_LOCK();
  if (sVolume) {
    if (pVolumeLabel) {
      pVolume = FS__FindVolume(sVolume, NULL);
      if (pVolume) {
        if ((FS__AutoMount(pVolume) & FS_MOUNT_R) == FS_MOUNT_R)  {
          FS_LOCK_DRIVER(&pVolume->Partition.Device);
          r = FS_GET_VOLUME_LABEL(pVolume, pVolumeLabel, VolumeLabelSize);
          FS_UNLOCK_DRIVER(&pVolume->Partition.Device);
        }
      }
    }
  }
  FS_UNLOCK();
  return r;
}

/*********************************************************************
*
*       FS_SetVolumeLabel
*
*  Description:
*    Returns a volume label name if one exists.
*
*  Parameters:
*    sVolume            - The volume name
*    pVolumeLabel       - Pointer to a buffer with the new volume label.
*                       - NULL indicates, that the volume label should
*                         be deleted.
*
*  Return value:
*    0     - Success.
*   -1     - Error.
*/
int FS_SetVolumeLabel(const char * sVolume, const char * pVolumeLabel) {
  int  r;
  FS_VOLUME * pVolume;
  r = -1;          /* Error - Failed to get volume information */
  FS_LOCK();
  if (sVolume) {
    pVolume = FS__FindVolume(sVolume, NULL);
    if (pVolume) {
      if (FS__AutoMount(pVolume) != FS_MOUNT_RW)  {
        FS_LOCK_DRIVER(&pVolume->Partition.Device);
        FS_JOURNAL_BEGIN(pVolume);
        r = FS_SET_VOLUME_LABEL(pVolume, pVolumeLabel);
        FS_JOURNAL_END(pVolume);
        FS_UNLOCK_DRIVER(&pVolume->Partition.Device);
      }
    }
  }
  FS_UNLOCK();
  return r;
}

/*********************************************************************
*
*       FS_FindVolume
*
*  Description:
*    Finds a volume based on the fully qualified filename.
*    Format needs to be "device:<unit>", e.g. "nand:0" or "nand:"
*
*/
FS_VOLUME * FS_FindVolume(const char * sVolume) {
  const char * s;
  const char * sDevice;
  FS_VOLUME  * pVolume;
  int          i;
  unsigned     DeviceNameLen;
  unsigned     Unit;

  pVolume = &FS__aVolume[0];
  if (sVolume) {
    s = FS__strchr(sVolume, ':');
    DeviceNameLen = s - sVolume;
    if (s) {
      i = 0;
      Unit = *(s + 1) - '0';
      if (Unit > 9) {
        Unit = 0;
      }
      do {
        const FS_DEVICE_TYPE * pDevice;
        FS_DEVICE_DATA       * pDevData;

        pDevice  = pVolume->Partition.Device.pType;
        pDevData = &pVolume->Partition.Device.Data;
        if (i >= _NumVolumes) {
          pVolume = (FS_VOLUME *)NULL;
          break;                                         /* No matching device found */
        }
        if (pDevData->Unit == Unit) {
          sDevice = pDevice->pfGetName(pDevData->Unit);
          if (strlen(sDevice) == DeviceNameLen) {
            if (FS_MEMCMP(sDevice, sVolume, DeviceNameLen) == 0) {
              break;                                       // Found device
            }
          }
        }
        pVolume++;
        i++;
      } while (1);
    }
  }
  return pVolume;
}

/*********************************************************************
*
*       FS_GetVolumeStatusByFile
*
*  Function description:
*    Returns the status of a volume.
*
*  Parameters
*    pFile           - Pointer to a FS_FILE.
*
*  Return value:
*     FS_MEDIA_NOT_PRESENT     - Volume is not present.
*     FS_MEDIA_IS_PRESENT      - Volume is present.
*     FS_MEDIA_STATE_UNKNOWN   - Volume state is unknown.
*
*/
int	FS_GetVolumeStatusByFile(FS_FILE *pFile)
{
	if(pFile)
	{
		FS_VOLUME *pVolume;
		 int         r;
		
		FS_LOCK();
		pVolume = pFile->pFileObj->pVolume;
		r = FS__GetVolumeStatus(pVolume);
		FS_UNLOCK();
		return r;
	}
	else
	{
		return FS_MEDIA_NOT_PRESENT;
	}
}

/*********************************************************************
*
*       FS_Mount
*
*  Function description:
*    Mounts a volume, if necessary.
*
*  Return value:
*      0     - Success.
*   != 0     - Error.
*/
int FS_Mount(const char * sVolume) {
  FS_VOLUME * pVolume;
  int         r;

  FS_LOCK();
  r       = 1;  // Set as error so far
  pVolume = FS__FindVolume(sVolume, NULL);
  if (pVolume) {
    if (FS__Mount(pVolume, FS_MOUNT_RW) > 0) {
      r = 0;
    }
  }
  FS_UNLOCK();
  return r;
}

/*********************************************************************
*
*       FS_MountEx
*
*  Function description:
*    Mounts a volume, if necessary.
*
*  Parameters:
*    sVolume            - The volume name
*    MountType          - Sho
*
*  Return value:
*      0     - Success.
*   != 0     - Error.
*/
int FS_MountEx(const char * sVolume, U8 MountType) {
  FS_VOLUME * pVolume;
  int         r;

  FS_LOCK();
  r       = 1;  // Set as error so far
  pVolume = FS__FindVolume(sVolume, NULL);
  if (pVolume) {
    r = FS__Mount(pVolume, MountType);
  }
  FS_UNLOCK();
  return r;
}


/*********************************************************************
*
*       FS_IsLLFormatted
*
*  Function description:
*    Returns if a volume is low-level formatted or not.
*
*  Return value:
*      1     - Volume is low-level formatted.
*      0     - Volume is not low-level formatted.
*     -1     - Low level format not supported by volume.
*/
int FS_IsLLFormatted(const char * sVolume) {
  FS_VOLUME * pVolume;
  int         r;

  FS_LOCK();
  r       = -1;  // Set as error so far
  pVolume = FS__FindVolume(sVolume, NULL);
  if (pVolume) {
    r = FS__IsLLFormatted(pVolume);
  }
  FS_UNLOCK();
  return r;
}

/*********************************************************************
*
*       FS_IsHLFormatted
*
*  Function description:
*    Returns if a volume is high-level formatted or not.
*
*  Return value:
*      1     - Volume is     high-level formatted.
*      0     - Volume is not high-level formatted.
*     -1     - Device is not ready or general error.
*/
int FS_IsHLFormatted(const char * sVolume) {
  FS_VOLUME * pVolume;
  int         r;

  FS_LOCK();
  r       = -1;  // Set as error so far
  pVolume = FS__FindVolume(sVolume, NULL);
  if (pVolume) {
    r = FS__AutoMount(pVolume);
    if (r > 0) {
      r = 1;
    }
  }
  FS_UNLOCK();
  return r;
}

/*********************************************************************
*
*       FS_CheckDisk
*
*  Function description
*     This function checks the file system for corruption:
*     The following corruption are detected/fixed:
*       * Invalid directory entries.
*       * Lost clusters/cluster chains.
*       * Cross linked clusters.
*       * Clusters are associated to a file with size of 0.
*       * Too few clusters are allocated to a file.
*       * Cluster is not marked as end-of-chain, although it should be.
*
*  Parameters
*    sVolume           - Pointer to a string containing the name of the volume.
*    pBuffer           - Pointer to a buffer that shall be used for checking the cluster entries.
*    BufferSize        - Size of the buffer in bytes.
*    MaxRecursionLevel - The max recursion depth checkdisk shall go.
*    pfOnError         - Pointer to a callback function that shall report the user of the error. NULL is not permitted, but returns an error.
*
*  Return value
*    0    O.K.  - File system is not in a corrupted state.
*    1    Error -> an error has be found and repaired, retry is required.
*    2    User specified an abort of checkdisk operation thru callback or volume not mountable.
*/
int FS_CheckDisk(const char * sVolumeName, void *pBuffer, U32 BufferSize, int MaxRecursionLevel, FS_QUERY_F_TYPE * pfOnError) {
  FS_VOLUME    * pVolume;
  FS_DISK_INFO   DiskInfo;
  int            r = 2;

  if (pfOnError == NULL) {
    FS_DEBUG_ERROROUT("No callback for error reporting is specified, returning");
    return 2;
  }
  FS_LOCK();
  pVolume = FS__FindVolume(sVolumeName, NULL);
  if (pVolume) {
    FS__Unmount(pVolume);
    r = FS__GetVolumeInfoEx(pVolume, &DiskInfo);
    if (r == 0) {
      FS_LOCK_DRIVER(&pVolume->Partition.Device);
      FS_JOURNAL_INVALIDATE(pVolume);
      r = FS_CHECKDISK(pVolume, &DiskInfo, pBuffer, BufferSize, MaxRecursionLevel, pfOnError);
      if (r == 0) {
        FS_JOURNAL_MOUNT(pVolume);
      }
      FS_UNLOCK_DRIVER(&pVolume->Partition.Device);
    } else {
      FS_DEBUG_ERROROUT("Medium does not contain a valid EFS allocation table structure.");
      r = 2;
    }
  }
  FS_UNLOCK();
  return r;
}

/*********************************************************************
*
*       FS_SetAutoMount
*
*  Function description:
*    Sets the mount behaviour of the specified volume.
*
*  Parameters
*    sVolume           - Pointer to a string containing the name of the volume.
*    MountType         - 3 values are allowed:
*                          FS_MOUNT_R    - Allows to auto mount the volume read only.
*                          FS_MOUNT_RW   - Allows to auto mount the volume read/write.
*                          0             - Disables auto mount for the volume.
*
*/
void FS_SetAutoMount(const char  * sVolume, U8 MountType) {
 FS_VOLUME * pVolume;

 FS_LOCK();
 pVolume = FS__FindVolume(sVolume, NULL);
 if (pVolume) {
   FS_LOCK_SYS();
   pVolume->AllowAutoMount = MountType;
   FS_UNLOCK_SYS();
 }
 FS_UNLOCK();

}

/*********************************************************************
*
*       FS_GetVolumeStatus
*
*  Function description:
*    Returns the status of a volume.
*
*  Parameters
*    sVolume           - Pointer to a string containing the name of the volume.
*
*  Return value:
*     FS_MEDIA_NOT_PRESENT     - Volume is not present.
*     FS_MEDIA_IS_PRESENT      - Volume is present.
*     FS_MEDIA_STATE_UNKNOWN   - Volume state is unknown.
*
*/
int FS_GetVolumeStatus(const char  * sVolume) {
 FS_VOLUME * pVolume;
 int         r;

 FS_LOCK();
 pVolume = FS__FindVolume(sVolume, NULL);
 r = FS__GetVolumeStatus(pVolume);
 FS_UNLOCK();
 return r;

}



void FS_VolumeSupportSpecialCache (const char *sVolume)
{
	FS_VOLUME * pVolume;

	FS_LOCK();
	 pVolume = FS__FindVolume(sVolume, NULL);
	if (pVolume) {
		FS_LOCK_SYS();
		pVolume->SupportSpecialCache = 1;
		FS_UNLOCK_SYS();
	}
	FS_UNLOCK();
}

/*************************** End of file ****************************/
