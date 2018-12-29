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
File        : FS_SetFilePos.c
Purpose     : Implementation of FS_SetFilePos
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
*       Public code
*
**********************************************************************
*/


/*********************************************************************
*
*       FS_SetFilePos
*
*  Function description:
*    Set current position of a file handle.
*
*  Parameters:
*    pFile           - Pointer to a FS_FILE data structure.
*    DistanceToMove  - Offset for setting the file pointer position.
*    MoveMethod      - Starting point for the file pointer move.
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
int FS_SetFilePos(FS_FILE * pFile, I32 DistanceToMove, int MoveMethod) {
  int r;
  FS_LOCK();
  r = FS__FSeek(pFile, DistanceToMove, MoveMethod);
  FS_UNLOCK();
  return r;
}

/*************************** End of file ****************************/
