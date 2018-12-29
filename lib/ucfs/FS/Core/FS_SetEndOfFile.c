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
File        : FS_SetEndOfFile.c
Purpose     : Implementation of FS_SetEndOfFile
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/
#include "FS_ConfDefaults.h"        /* FS Configuration */
#include "FS_Int.h"
#include "FS_OS.h"
#include "FS_CLib.h"


/*********************************************************************
*
*       Public code internal
*
**********************************************************************
*/

/*********************************************************************
*
*       FS__SetEndOfFile
*
*  Function description:
*    Internal version of the FS_SetEndOfFile
*
*  Parameters:
*    pFile           - Pointer to a FS_FILE data structure.
*
*  Return value:
*    == 0        - New End of File has been set.
*    ==-1        - An error has occured.
*
*  Notes
*    (1) Move or copy
*        If the files are on the same volume, the file is moved,
*        otherwise copied and the original deleted.
*/
int FS__SetEndOfFile(FS_FILE * pFile) {
  FS_VOLUME * pVolume;
  int            r;

  if ((pFile->AccessFlags & FS_FILE_ACCESS_FLAGS_ACW)) {
    pVolume = pFile->pFileObj->pVolume;
    FS_LOCK_DRIVER(&pFile->pFileObj->pVolume->Partition.Device);
    FS_JOURNAL_BEGIN (pVolume);
    r = FS_SET_END_OF_FILE(pFile);
    FS_JOURNAL_END  (pVolume);
    FS_UNLOCK_DRIVER(&pFile->pFileObj->pVolume->Partition.Device);
  } else {
    FS_DEBUG_ERROROUT("FS__SetEndOfFile: pFile does not have write permission.");
    r = -1;
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
*       FS_SetEndOfFile
*
*  Function description:
*    Sets current position to the end of file.
*
*  Parameters:
*    pFile           - Pointer to a FS_FILE data structure.
*
*  Return value:
*    == 0        - New End of File has been set.
*    ==-1        - An error has occured.
*
*  Notes
*    (1) Move or copy
*        If the files are on the same volume, the file is moved,
*        otherwise copied and the original deleted.
*/
int FS_SetEndOfFile(FS_FILE * pFile) {
  int           r;

  r = -1;
  FS_LOCK();
  if (pFile) {
    r = FS__SetEndOfFile(pFile);
  }
  FS_UNLOCK();
  return r;
}

/*************************** End of file ****************************/
