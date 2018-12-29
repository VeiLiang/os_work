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
File        : FS_Truncate.c
Purpose     : Implementation of FS_Truncate
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
*       Public code
*
**********************************************************************
*/


/*********************************************************************
*
*       FS_Truncate
*
*  Function description:
*    Truncates a open file.
*
*  Parameters:
*    pFile        - Pointer to a valid opened file with write access.
*    NewSize      - New size of the file.
*
*  Return value:
*    == 0        - File has been truncated.
*    ==-1        - An error has occured.
*
*  Notes
*    (1) Move or copy
*        If the files are on the same volume, the file is moved,
*        otherwise copied and the original deleted.
*/
int FS_Truncate(FS_FILE * pFile, U32 NewSize) {
  int           r;
  FS_LOCK();
  r = -1;
  if (pFile->pFileObj->Size > NewSize) {
    FS__FSeek(pFile, (I32)NewSize, FS_FILE_BEGIN);
    r = FS__SetEndOfFile(pFile);
  }
  FS_UNLOCK();
  return r;
}

/*************************** End of file ****************************/
