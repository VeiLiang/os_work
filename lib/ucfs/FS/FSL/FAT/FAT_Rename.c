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
File        : FS_FAT_Rename.c
Purpose     : FAT routines for renaming files or directories
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "FAT_Intern.h"


/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       FS_FAT_Rename
*
*  Description:
*    Rename a existing file/directory.
*
*  Parameters:
*    sOldName    - path to the file/directory.
*    sNewName    - new file/directory name.
*    pVolume     - Pointer to an FS_VOLUME data structure.
*
*  Return value:
*    0         O.K.
*    1         Error
*/
int FS_FAT_Rename(const char * sOldName, const char * sNewName, FS_VOLUME * pVolume) {
  const char    *  pOldName;
  int              r;
  FS_SB            SB;
  U32              DirStart;

  r = 1;       /* No error so far */
  /*
   * Search directory
   */
  FS__SB_Create(&SB, &pVolume->Partition);
  if (FS_FAT_FindPath(pVolume, &SB, sOldName, &pOldName, &DirStart)) {
    //
    // check if entry exists
    //
    r = FS__FAT_Move(pVolume, DirStart, DirStart, pOldName, sNewName, &SB);
  }
  FS__SB_Delete(&SB);
  return r;
}

/*************************** End of file ****************************/
