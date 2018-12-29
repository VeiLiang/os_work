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
File        : FS_FWrite.c
Purpose     : Implementation of the said function.
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
*       FS_FWrite
*
*  Function description
*    Write data to a file.
*
*  Parameters
*    pData       - Pointer to a data to be written to a file.
*    Size        - Size of an element to be transferred.
*    N           - Number of elements to be transferred to the file.
*    pFile       - Pointer to a FS_FILE data structure.
*
*  Return value
*    Number of elements written.
*/
U32 FS_FWrite(const void *pData, U32 Size, U32 N, FS_FILE *pFile) {
  U32 NumBytesWritten;
  U32 NumBytes;

  FS_LOCK();
  NumBytesWritten = 0;
  if (Size == 0) {
    FS_UNLOCK();
    return 0;  /* if zero-size return with zero since we would divide by zero at the end of the function */
  }
  NumBytes = N * Size;
  NumBytesWritten = FS__Write(pFile, pData, NumBytes);
  FS_UNLOCK();
  return NumBytesWritten / Size;
}

/*************************** End of file ****************************/
