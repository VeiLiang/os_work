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
File        : FS_memcpy.c
Purpose     : Implementation of said function
---------------------------END-OF-HEADER------------------------------
*/

#include <string.h>
#include "FS_Int.h"

/*********************************************************************
*
*       public code
*
**********************************************************************
*/
/*********************************************************************
*
*       FS_memcpy
*
* Purpose:
*  Replacement for the memset function. The advantage is high speed
*  on all systems (sometime up to 10 times as fast as the one
*  in the library)
*  Main idea is to write int-wise.
*/
void FS_memcpy(void * pDest, const void * pSrc, int NumBytes) {
  char * pd;
  char * ps;
  pd = (char*)pDest;
  ps = (char*)pSrc;
  //
  // Copy words if possible
  //
  if ((((U32)ps & 3) == 0) && (((U32)pd & 3) == 0)) {
    unsigned NumWords = NumBytes >> 2;
    while (NumWords >= 4) {
      *(int*)pd = *(int*)ps;
      pd += 4;
      ps += 4;
      *(int*)pd = *(int*)ps;
      pd += 4;
      ps += 4;
      *(int*)pd = *(int*)ps;
      pd += 4;
      ps += 4;
      *(int*)pd = *(int*)ps;
      pd += 4;
      ps += 4;
      NumWords -= 4;
    }
    if (NumWords) {
      do {
        *(int*)pd = *(int*)ps;
        pd += 4;
        ps += 4;
      } while (--NumWords);
    }
    NumBytes &= 3;
  }
  //
  // Copy halfwords if possible
  //
  if ((((U32)ps & 1) == 0) && (((U32)pd & 1) == 0)) {
    unsigned NumHWords = NumBytes >> 1;
    while (NumHWords >= 4) {
      *(short*)pd = *(short*)ps;
      pd += 2;
      ps += 2;
      *(short*)pd = *(short*)ps;
      pd += 2;
      ps += 2;
      *(short*)pd = *(short*)ps;
      pd += 2;
      ps += 2;
      *(short*)pd = *(short*)ps;
      pd += 2;
      ps += 2;
      NumHWords -= 4;
    }
    if (NumHWords) {
      do {
        *(short*)pd = *(short*)ps;
        pd += 2;
        ps += 2;
      } while (--NumHWords);
    }
    NumBytes &= 1;
  }
  //
  // Copy bytes, bulk
  //
  while (NumBytes >= 4) {
    *(char*)pd++ = *(char*)ps++;
    *(char*)pd++ = *(char*)ps++;
    *(char*)pd++ = *(char*)ps++;
    *(char*)pd++ = *(char*)ps++;
    NumBytes -= 4;
  };
  //
  // Copy bytes, one at a time
  //
  if (NumBytes) {
    do {
      *(char*)pd++ = *(char*)ps++;
    } while (--NumBytes);
  };
}

/*************************** End of file ****************************/
