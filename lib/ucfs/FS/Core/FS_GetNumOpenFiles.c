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
File        : FS_GetNumOpenFiles.c
Purpose     : Implementation of FS_GetNumOpenFiles
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
*       FS_GetNumOpenFiles
*
*  Function description:
*    Returns how many files are currently opened.
*
*  Parameters:
*    None
*
*
*  Return value:
*      Number of files that are opened.
*
*
*/
int FS_GetNumFilesOpen(void) {
  int r;
  unsigned i;

  FS_LOCK();
  r = 0;
  FS_LOCK_SYS();
  for (i =0; i < COUNTOF(FS__aFilehandle); i++) { /* While no free entry found. */
    if (FS__aFilehandle[i].InUse) {
      r++;
    }
  }
  FS_UNLOCK_SYS();
  FS_UNLOCK();
  return r;
}

/*************************** End of file ****************************/
