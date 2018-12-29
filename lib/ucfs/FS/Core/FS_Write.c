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
File        : FS_Write.c
Purpose     : Implementation of FS_Write
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdio.h>
#include "FS_Int.h"
#include "FS_OS.h"

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       FS_Write
*
*  Function description:
*    Internal version of FS_Write.
*    Write data to a file.
*
*  Parameters:
*    pFile       - Pointer to a FS_FILE data structure.
*    pData       - Pointer to the data
*    NumBytes    - Number of bytes to be written
*
*  Return value:
*    Number of bytes written.
*/
U32 FS__Write(FS_FILE *pFile, const void *pData, U32 NumBytes) {
  char          InUse;
  U32           NumBytesWritten;
  FS_FILE_OBJ * pFileObj;
  FS_DEVICE   * pDevice = NULL;

  if (NumBytes == 0) {
    return 0;
  }
  if (pFile == NULL) {
    return 0;  /* No pointer to a FS_FILE structure */
  }
  NumBytesWritten = 0;
  //
  // Load file information
  //
  FS_LOCK_SYS();
  InUse   = pFile->InUse;
  pFileObj = pFile->pFileObj;
  if (pFileObj) {
    if (pFileObj->pVolume) {
      pDevice = &pFileObj->pVolume->Partition.Device;
    }
  }
  FS_UNLOCK_SYS();
  if ((InUse == 0) || (pFileObj == (FS_FILE_OBJ*)NULL)) {
    FS_DEBUG_ERROROUT("Application error: FS__Write: Invalid file handle");
    return 0;
  }
  if (pDevice == NULL) {
    FS_DEBUG_ERROROUT("Application error: FS__Write: volume already unmounted");
    return 0;
  }
  //
  // Lock driver before performing operation
  //
  FS_LOCK_DRIVER(pDevice);
  //
  // Multi-tasking environments with per-driver-locking:
  // Make sure that relevant file information has not changed (an other task may have closed the file, unmounted the volume etc.)
  // If it has, no action is performed.
  //
#if FS_OS_LOCK_PER_DRIVER
  FS_LOCK_SYS();
  if (pFileObj != pFile->pFileObj || (pFile->pFileObj == NULL)) {
    InUse = 0;
  }
  if (pFile->InUse == 0) {
    InUse = 0;
  }
  FS_UNLOCK_SYS();
  if (InUse == 0) {      // Let's make sure the file is still valid
    FS_DEBUG_ERROROUT("Application error: File handle has been invalidated by other thread during wait");
  } else
#endif
  //
  // All checks and locking operations completed. Call the File system (FAT/EFS) layer.
  //
  {
    NumBytesWritten = 0;
    if ((pFile->AccessFlags & FS_FILE_ACCESS_FLAG_W) == 0) {
      pFile->Error    = FS_ERR_READONLY;             /* Open mode does now allow write access */
    } else {
#if FS_SUPPORT_JOURNAL
      if (pFileObj->pVolume->Partition.Device.Data.JournalIsActive == 1) {
        FS_VOLUME * pVolume;
        pVolume = pFileObj->pVolume;
        do {
          U32 NumBytesWrittenAtOnce;
          U32 NumBytesAtOnce;
          I32 SpaceInJournal;

          SpaceInJournal  = (FS_JOURNAL_GetNumFreeSectors(pVolume) - 2) * 15 / 16;   // Reserve 2 sectors and about 8% for management
          if (SpaceInJournal <= 0) {
            NumBytesWritten = FS_FWRITE(pFile, pData, NumBytes); /* Execute the FSL function */
            FS_DEBUG_ERROROUT("Insufficient space in journal!");
            break;
          }
          SpaceInJournal *= pVolume->FSInfo.Info.BytesPerSector;                     // Convert number of sectors into number of bytes
          NumBytesAtOnce = MIN((U32)SpaceInJournal, NumBytes);

          FS_JOURNAL_BEGIN (pFileObj->pVolume);
          NumBytesWrittenAtOnce = FS_FWRITE(pFile, pData, NumBytesAtOnce); /* Execute the FSL function */
          FS_JOURNAL_END   (pFileObj->pVolume);
          NumBytesWritten += NumBytesWrittenAtOnce;
          if (NumBytesWrittenAtOnce != NumBytesAtOnce) {
            break;
          }
          NumBytes -= NumBytesAtOnce;
          pData     = NumBytesAtOnce + (const U8*)pData;
        } while (NumBytes);
      } else
#endif
      {
        NumBytesWritten = FS_FWRITE(pFile, pData, NumBytes); /* Execute the FSL function */
      }
    }
  }

  FS_UNLOCK_DRIVER(pDevice);
  return NumBytesWritten;
}

/*********************************************************************
*
*       FS_Write
*
*  Function description:
*    API function. Write data to a file.
*
*  Parameters:
*    pFile       - Pointer to a FS_FILE data structure.
*    pData       - Pointer to the data
*    NumBytes    - Number of bytes to be written
*
*  Return value:
*    Number of bytes written.
*/
U32 FS_Write(FS_FILE *pFile, const void *pData, U32 NumBytes) {
  U32 NumBytesWritten;

  FS_LOCK();
  NumBytesWritten = FS__Write(pFile, pData, NumBytes);
  FS_UNLOCK();
  return NumBytesWritten;
}

/*************************** End of file ****************************/
