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
File        : FS_ErrorOut.C
Purpose     : Logging (used only at higher debug levels)
---------------------------END-OF-HEADER------------------------------
*/

#include <stdio.h>
#include <string.h>
#include "FS_Int.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#define MAXLEN 50

/*********************************************************************
*
*      Static code
*
**********************************************************************
*/
/*********************************************************************
*
*       _CopyString
*/
static void _CopyString(char*d, const char*s, int MaxLen) {
  while ((MaxLen > 0) && *s) {
    *d++ = *s++;
    MaxLen--;
  }
  *d = 0;
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
/*********************************************************************
*
*       FS__ErrorOut
*/
void FS__ErrorOut(const char *s) { FS_X_ErrorOut(s); }

/*********************************************************************
*
*       FS__ErrorOut1
*/
void FS__ErrorOut1(const char *s, int p0) {
  char ac[MAXLEN + 10];
  char* sOut = ac;
  FS_MEMSET((U8*)ac, 0, sizeof(ac));
  _CopyString(ac, s, MAXLEN);
  sOut += FS_STRLEN(sOut);
  FS__AddSpaceHex(p0, 8, &sOut);
  FS__ErrorOut(ac);
}

/*********************************************************************
*
*       FS__ErrorOut2
*/
void FS__ErrorOut2(const char *s, int p0, int p1) {
  char ac[MAXLEN + 20];
  char* sOut = ac;
  FS_MEMSET((U8*)ac, 0, sizeof(ac));
  _CopyString(ac, s, MAXLEN);
  sOut += FS_STRLEN(sOut);
  FS__AddSpaceHex(p0, 8, &sOut);
  FS__AddSpaceHex(p1, 8, &sOut);
  FS__ErrorOut(ac);
}

/*********************************************************************
*
*       FS__ErrorOut3
*/
void FS__ErrorOut3(const char *s, int p0, int p1, int p2) {
  char ac[MAXLEN + 30];
  char* sOut = ac;
  FS_MEMSET((U8*)ac, 0, sizeof(ac));
  _CopyString(ac, s, MAXLEN);
  sOut += FS_STRLEN(sOut);
  FS__AddSpaceHex(p0, 8, &sOut);
  FS__AddSpaceHex(p1, 8, &sOut);
  FS__AddSpaceHex(p2, 8, &sOut);
  FS__ErrorOut(ac);
}

/*********************************************************************
*
*       FS__ErrorOut4
*/
void FS__ErrorOut4(const char *s, int p0, int p1, int p2, int p3) {
  char ac[MAXLEN + 40] = {0};
  char* sOut = ac;
  FS_MEMSET((U8*)ac, 0, sizeof(ac));
  _CopyString(ac, s, MAXLEN);
  sOut += FS_STRLEN(sOut);
  FS__AddSpaceHex(p0, 8, &sOut);
  FS__AddSpaceHex(p1, 8, &sOut);
  FS__AddSpaceHex(p2, 8, &sOut);
  FS__AddSpaceHex(p3, 8, &sOut);
  FS__ErrorOut(ac);
}

/*************************** End of file ****************************/
