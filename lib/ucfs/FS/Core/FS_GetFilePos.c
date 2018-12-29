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
File        : FS_GetFilePos.c
Purpose     : Implementation of FS_GetFilePos
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
*       FS_GetFilePos
*
*  Function description:
*    Returns the current position of a file handle.
*
*  Parameters:
*    pFile         - Pointer to a FS_FILE data structure.
*
*  Return value:
*    >=0           - Current position of the file pointer.
*    ==-1          - An error has occured.
*
*/
I32 FS_GetFilePos(FS_FILE * pFile) {
  int r;
  FS_LOCK();
  r = FS__FTell(pFile);
  FS_UNLOCK();
  return r;
}

/*************************** End of file ****************************/
