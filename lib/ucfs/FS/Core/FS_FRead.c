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
File        : FS_FRead.c
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
*       FS_FRead
*
*  Function description
*    Reads data from a open file.
*
*  Parameters
*    pData       - Pointer to a data buffer for storing data transferred
*                  from file.
*    Size        - Size of an element to be transferred from file to data
*                  buffer
*    N           - Number of elements to be transferred from the file.
*    pFile       - Pointer to a FS_FILE data structure.
*
*  Return value
*    Number of elements read.
*/
U32 FS_FRead(void *pData, U32 Size, U32 N, FS_FILE *pFile) {
  U32 i;
  U32 NumBytes;

  FS_LOCK();
  if (Size == 0)  {
    FS_UNLOCK();
    return 0;
  }
  NumBytes = N * Size;
  i = FS__Read(pFile, pData, NumBytes);
  FS_UNLOCK();
  return (i / Size);
}

/*************************** End of file ****************************/
