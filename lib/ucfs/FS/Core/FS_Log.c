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
---------------------------------------------------------------------
File        : FS_Log.c
Purpose     : Logging (used only at higher debug levels)
---------------------------END-OF-HEADER------------------------------
*/

#include <stdio.h>
#include <string.h>
#include "FS_Int.h"

/*********************************************************************
*
*      defines
*
**********************************************************************
*/

#define MAXLEN 50

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/
/*********************************************************************
*
*       _CopyString
*/
static void _CopyString(char* d, const char* s, int MaxLen) {
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
*  Note: These routines are needed only in higher debug levels.
*
**********************************************************************
*/
/*********************************************************************
*
*       FS__Log
*/
void FS__Log(const char *s) {
  FS_X_Log(s);
}

/*********************************************************************
*
*       FS__Log1
*/
void FS__Log1(const char *s, int p0) {
  char ac[MAXLEN + 10];
  char* sOut = ac;
  _CopyString(ac, s, MAXLEN);
  sOut += FS_STRLEN(sOut);
  FS__AddSpaceHex(p0, 8, &sOut);
  FS__Log(ac);
}

/*********************************************************************
*
*       FS__Log2
*/
void FS__Log2(const char *s, int p0, int p1) {
  char ac[MAXLEN + 20];
  char* sOut = ac;
  _CopyString(ac, s, MAXLEN);
  sOut += FS_STRLEN(sOut);
  FS__AddSpaceHex(p0, 8, &sOut);
  FS__AddSpaceHex(p1, 8, &sOut);
  FS__Log(ac);
}

/*********************************************************************
*
*       FS__Log3
*/
void FS__Log3(const char *s, int p0, int p1, int p2) {
  char ac[MAXLEN + 30];
  char* sOut = ac;
  _CopyString(ac, s, MAXLEN);
  sOut += FS_STRLEN(sOut);
  FS__AddSpaceHex(p0, 8, &sOut);
  FS__AddSpaceHex(p1, 8, &sOut);
  FS__AddSpaceHex(p2, 8, &sOut);
  FS__Log(ac);
}

/*********************************************************************
*
*       FS__Log4
*/
void FS__Log4(const char *s, int p0, int p1, int p2, int p3) {
  char ac[MAXLEN + 40];
  char* sOut = ac;
  _CopyString(ac, s, MAXLEN);
  sOut += FS_STRLEN(sOut);
  FS__AddSpaceHex(p0, 8, &sOut);
  FS__AddSpaceHex(p1, 8, &sOut);
  FS__AddSpaceHex(p2, 8, &sOut);
  FS__AddSpaceHex(p3, 8, &sOut);
  FS__Log(ac);
}

/*************************** End of file ****************************/
