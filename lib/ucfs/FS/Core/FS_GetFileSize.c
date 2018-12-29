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
File        : FS_GetFileSize.c
Purpose     : Implementation of FS_GetFileSize
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
*       Public code, internal
*
**********************************************************************
*/

/*********************************************************************
*
*       FS__GetFileSize
*
*  Function description:
*    Internal version of FS_GetFileSize.
*    Returns the size of a file
*
*  Parameters:
*    pFile       - Pointer to a FS_FILE data structure.
*                  The file must have been opened with read or write access.
*
*  Return value:
*    0xFFFFFFFF     - Indicates failure
*    0 - 0xFFFFFFFE - File size of the given file
*
*/
U32 FS__GetFileSize(FS_FILE *pFile) {
  U32 r;

  r = 0xFFFFFFFF;                      /* Error value */
  if (pFile) {
    FS_LOCK_SYS();
    r = pFile->pFileObj->Size;
    FS_UNLOCK_SYS();
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
*       FS_GetFileSize
*
*  Function description:
*    Returns the size of a file
*
*  Parameters:
*    pFile       - Pointer to a FS_FILE data structure.
*                  The file must have been opened with read or write access.
*
*  Return value:
*    0xFFFFFFFF     - Indicates failure
*    0 - 0xFFFFFFFE - File size of the given file
*
*/
U32 FS_GetFileSize(FS_FILE *pFile) {
  U32 r;

  FS_LOCK();
  r = FS__GetFileSize(pFile);
  FS_UNLOCK();
  return r;
}


/*************************** End of file ****************************/
