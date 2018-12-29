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
File        : FAT_API.c
Purpose     : FAT File System Layer function table
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*             #include Section
*
**********************************************************************
*/

#include "FAT.h"
#include "FAT_Intern.h"

#if FS_SUPPORT_MULTIPLE_FS
/*********************************************************************
*
*       Public const
*
**********************************************************************
*/
const FS_FS_API FS_FAT_API = {
  &FS_FAT_ReadBPB,
  &FS_FAT_Open,
  &FS_FAT_Close,
  &FS_FAT_Read,
  &FS_FAT_Write,
  &FS_FAT_OpenDir,
  &FS_FAT_CloseDir,
  &FS_FAT_ReadDir,
  &FS_FAT_RemoveDir,
  &FS_FAT_CreateDir,
  &FS_FAT_Rename,
  &FS_FAT_Move,
  &FS_FAT_SetDirEntryInfo,
  &FS_FAT_GetDirEntryInfo,
  &FS_FAT_SetEndOfFile,
  &FS_FAT_Unmount
};
#else
  void FAT_API_c(void);
  void FAT_API_c(void){}
#endif

/*************************** End of file ****************************/
