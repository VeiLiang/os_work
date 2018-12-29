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
File        : FS_FAT_Move.c
Purpose     : FAT routines for moving files or directory
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*             #include Section
*
**********************************************************************
*/

#include "FAT_Intern.h"

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       FS__FAT_Move
*
*/
char FS__FAT_Move(FS_VOLUME * pVolume, U32 DirStartOld, U32 DirStartNew, const char * sOldName, const char * sNewName, FS_SB * pSB) {
  U16             Date;
  U16             Time;
  U32             ClusterId;
  U32             Size;
  U8              Attrib;
  char            r;
  FS_FAT_DENTRY * pDirEntryCheck;
  FS_FAT_DENTRY * pDirEntry;
  int             LongDirEntryIndex = -1;
  U32             OldDirSectorNo;

  r                   = 1;
  //
  // Check at first if the directory entry already exists in the target directory
  //
  pDirEntryCheck = FS_FAT_FindDirEntry(pVolume, pSB, sNewName, 0, DirStartNew, 0, NULL);
  if(pDirEntryCheck) {
    FS_DEBUG_ERROROUT("FS__FAT_Move: source file already exist in target directory.");
    return 1;
  }
  //
  //  Find old entry
  //
  pDirEntry = FS_FAT_FindDirEntry(pVolume, pSB, sOldName, 0, DirStartOld, 0, &LongDirEntryIndex);
  if (pDirEntry == NULL) {
    FS_DEBUG_ERROROUT("FS__FAT_Move: source file/directory does not exist.");
    return 1;
  }
  //
  //  We need to remember the sector number of the old directory entry
  //  since we need to mark the directory entry as deleted.
  //
  OldDirSectorNo = pSB->SectorNo;
  //
  // Save the old directory entry
  //
  Attrib    = pDirEntry->data[DIR_ENTRY_OFF_ATTRIBUTES];
  Time      = FS_LoadU16LE(&pDirEntry->data[DIR_ENTRY_OFF_CREATION_TIME]);
  Date      = FS_LoadU16LE(&pDirEntry->data[DIR_ENTRY_OFF_CREATION_DATE]);
  ClusterId = FS_FAT_GetFirstCluster(pDirEntry);
  Size      = FS_LoadU32LE(&pDirEntry->data[DIR_ENTRY_OFF_SIZE]);
  if (Attrib & FS_FAT_ATTR_READ_ONLY) {
    FS_DEBUG_ERROROUT("FS__FAT_Move: source file/directory is read-only.");
    return 1;
  }
  //
  // Create the new directory entry
  //
  if (FAT_pDirEntryAPI->pfCreateDirEntry(pVolume, pSB, sNewName, DirStartNew, ClusterId, Attrib, Size, Time, Date)) {
    r = 0;
  }
  //
  //  In case of a directory we need to update the ".." directory entry.
  //
  if ((Attrib & FS_FAT_ATTR_DIRECTORY) == FS_FAT_ATTR_DIRECTORY) {
    FS_FAT_DENTRY * pSubDirEntry;
    pSubDirEntry = FS_FAT_FindDirEntry(pVolume, pSB, "..", 0, ClusterId, FS_FAT_ATTR_DIRECTORY, NULL);
    if (pSubDirEntry) {
      FS_FAT_WriteDirEntryCluster(pSubDirEntry, DirStartNew);
    }
    FS__SB_MarkDirty(pSB);
  }
  //
  // Set and mark old directory entry as invalid
  //
  FS__SB_SetSector(pSB, OldDirSectorNo, FS_SB_TYPE_DIRECTORY);
  FS__SB_Read(pSB);
  pDirEntry->data[0]  = 0xe5;
  FS__SB_MarkDirty(pSB);
  if (FAT_pDirEntryAPI->pfDelLongEntry) {
    FAT_pDirEntryAPI->pfDelLongEntry(pVolume, pSB, DirStartOld, LongDirEntryIndex);
  }
  return r;
}

/*********************************************************************
*
*       FS_FAT_Move
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
int FS_FAT_Move(const char * sOldName, const char * sNewName, FS_VOLUME * pVolume) {
  const char    *  pNewName;
  const char    *  pOldName;
  int              r;
  FS_SB            SB;
  U32              DirStartOld;
  U32              DirStartNew;
  /*
   * Search directory
   */
  FS__SB_Create(&SB, &pVolume->Partition);
  if (FS_FAT_FindPath(pVolume, &SB, sOldName, &pOldName, &DirStartOld) == 0) {
    FS__SB_Delete(&SB);
    return 1;  /* Directory not found */
  }
  if (FS_FAT_FindPath(pVolume, &SB, sNewName, &pNewName, &DirStartNew) == 0) {
    FS__SB_Delete(&SB);
    return 1;  /* Directory not found */
  }
  r = 1;       /* No error so far */
  /*
   * check if there is no entry with the new name
   */

  /*
   * check if entry exists
   */
  if (*pNewName == 0) {
    pNewName = pOldName;
  }
  r = FS__FAT_Move(pVolume, DirStartOld, DirStartNew, pOldName, pNewName, &SB);
  FS__SB_Delete(&SB);

  return r;
}

/*************************** End of file ****************************/
