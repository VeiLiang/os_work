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
File        : FS_Move.c
Purpose     : Implementation of FS_Move
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
*       FS_Move
*
*  Function description:
*    Moves a file/directory.
*
*  Parameters:
*    sFullNameSrc     - Fully qualified file name of the source file, e.g. ram:Src.txt
*    sFullNameDest    - Fully qualified file name of the source file, e.g. ram:Dest.txt
*
*  Return value:
*    ==0         - File has been renoved.
*    ==-1        - An error has occured.
*
*  Notes
*    (1) Move or copy
*        If the files are on the same volume, the file is moved,
*        otherwise copied and the original deleted.
*/
int FS_Move(const char * sFullNameSrc, const char * sFullNameDest) {
  const char * sPathNameSrc;
  const char * sPathNameDest;
  FS_VOLUME  * pVolumeSrc;
  FS_VOLUME  * pVolumeDest;
  int          r;

  FS_LOCK();
  r = -1;
  /* Find correct FSL  (device:unit:name) */
  pVolumeSrc  = FS__FindVolume(sFullNameSrc,  &sPathNameSrc );
  pVolumeDest = FS__FindVolume(sFullNameDest, &sPathNameDest);
  if (pVolumeSrc == pVolumeDest) {        /* Note 1 */
    if (FS__AutoMount(pVolumeSrc) == FS_MOUNT_RW)  {
      FS_LOCK_DRIVER(&pVolumeSrc->Partition.Device);
      FS_JOURNAL_BEGIN(pVolumeSrc);
      r = FS_MOVE(sPathNameSrc, sPathNameDest, pVolumeSrc);      /* Call the FSL function to move the file/directory  */
      FS_JOURNAL_END  (pVolumeSrc);
      FS_UNLOCK_DRIVER(&pVolumeSrc->Partition.Device);
      if (r) {
        r = -1;
      }
    }
  } else {
    if (FS__CopyFile(sFullNameSrc, sFullNameDest) == 0) {
      r = FS__Remove(sFullNameSrc);
    }
  }
  FS_UNLOCK();
  return r;
}

/*************************** End of file ****************************/
