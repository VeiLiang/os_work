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
File        : FS_Format.c
Purpose     : Implementation of the FS_Format API function.
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/


#include "FS_Int.h"

/*********************************************************************
*
*       Public code, internal
*
**********************************************************************
*/

/*********************************************************************
*
*       FS__Format
*
*  Function description:
*    Internal version of FS_Format.
*    Format the medium
*
*  Parameters:
*    pVolume       Volume to format. NULL is permitted, but returns an error.
*    pFormatInfo   Add. optional format information.
*
*  Return value:
*    ==0         - File system has been started.
*    !=0         - An error has occured.
*/
int FS__Format(FS_VOLUME  * pVolume, FS_FORMAT_INFO * pFormatInfo) {
  int          r;
  int          Status;
  FS_DEVICE  * pDevice;

  r = -1;
  if (pVolume) {
    pDevice = &pVolume->Partition.Device;
    FS_LOCK_DRIVER(pDevice);
    FS__UnmountForcedNL(pVolume);
    Status = FS_LB_GetStatus(pDevice);
    if (Status >= 0) {
      FS_LB_InitMediumIfRequired(pDevice);
      r = FS_FORMAT(pVolume, pFormatInfo);
    }
    FS_UNLOCK_DRIVER(pDevice);

  }
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
*       FS_Format
*
*  Function description:
*    Format the medium
*
*  Parameters:
*    pDevice       Device specifier (string). "" refers to the first device.
*    pFormatInfo   Add. optional format information.
*
*  Return value:
*    ==0         - O.K., format successful
*    !=0         - An error has occured.
*/
int FS_Format(const char *sVolumeName, FS_FORMAT_INFO * pFormatInfo) {
  int r;
  FS_VOLUME  * pVolume;

  FS_LOCK();
  pVolume = FS__FindVolume(sVolumeName, NULL);
  r = FS__Format(pVolume, pFormatInfo);
  FS_UNLOCK();
  return r;
}


/*********************************************************************
*
*       FS__FormatLow
*
*  Function description:
*    Low-level format the medium
*
*  Parameters:
*    pDevice       Device specifier (string). "" refers to the first device.
*
*  Return value:
*    ==0         - O.K.: Low level format successful
*    !=0         - ERROR
*/
int FS__FormatLow(FS_VOLUME * pVolume) {
  int          r;

  pVolume->IsMounted = 0;
  r = FS__IoCtl(pVolume, FS_CMD_FORMAT_LOW_LEVEL, 0, 0);  /* Erase & Low-level  format the flash */
  return r;
}

/*********************************************************************
*
*       FS_FormatLow
*
*  Function description:
*    Low-level format the medium
*
*  Parameters:
*    pDevice       Device specifier (string). "" refers to the first device.
*
*  Return value:
*    ==0         - O.K.: Low level format successful
*    !=0         - ERROR
*/
int FS_FormatLow(const char *pDevice) {
  int         r;
  FS_VOLUME * pVolume;

  FS_LOCK();
  r = -1;
  pVolume = FS__FindVolume(pDevice, NULL);
  if (pVolume) {
    r = FS__FormatLow(pVolume);
  }
  FS_UNLOCK();
  return r;
}


/*********************************************************************
*
*       FS_FormatLLIfRequired
*
*  Function description:
*    Low-level format the medium
*
*  Parameters:
*    pDevice       Device specifier (string). "" refers to the first device.
*
*  Return value:
*    == 0         - O.K.: Low level format successful.
*    == 1         - low-level format not required.
*    ==-1         - ERROR, low-level format not supported.
*/
int FS_FormatLLIfRequired(const char * sVolName) {
  int r;

  r = FS_IsLLFormatted(sVolName);
  if (r == 0) {
    FS_DEBUG_LOG("FS_FormatLLIfRequired: Low-level-formatting volume");
    r = FS_FormatLow(sVolName);
  }
  return r;
}

/*************************** End of file ****************************/
