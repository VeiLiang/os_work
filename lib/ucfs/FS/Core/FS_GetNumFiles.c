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
File        : FS_GetNumFiles.c
Purpose     : Implementation of FS_GetNumFiles
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

/*********************************************************************
*
*       Public code, internal
*
**********************************************************************
*/

/*********************************************************************
*
*       FS__GetNumFiles
*
*  Function description:
*    API function. Returns the size of a file
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
U32 FS__GetNumFiles(FS_DIR *pDir) {
  U32 r;

  if (pDir) {
    U16         EntryIndexOld;
    FS_DIR_POS  SectorPosOld;
    /* Save the old position in pDir structure */
    EntryIndexOld = pDir->Dir.DirEntryIndex;
    SectorPosOld = pDir->Dir.DirPos;
    FS__RewindDir(pDir);
    r = 0;
    do {
      U8         Attr;
      if (FS__ReadDir(pDir) == (FS_DIRENT *)NULL) {
        break; /* No more files */
      }
      FS__DirEnt2Attr(&pDir->DirEntry, &Attr);
      if (!(Attr & FS_ATTR_DIRECTORY)) { /* Is directory entry the volume ID  or a directory, ignore them */
        r++;
      }
    } while (1);
    /* Restore the old position in pDir structure */
    pDir->Dir.DirEntryIndex = EntryIndexOld;
    pDir->Dir.DirPos        = SectorPosOld;
  } else {
    r = 0xffffffff;
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
*       FS_GetNumFiles
*
*  Function description:
*    API function. Returns the size of a file
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
U32 FS_GetNumFiles(FS_DIR *pDir) {
  U32 r;

  FS_LOCK();
  r = FS__GetNumFiles(pDir);
  FS_UNLOCK();
  return r;
}

/*************************** End of file ****************************/
