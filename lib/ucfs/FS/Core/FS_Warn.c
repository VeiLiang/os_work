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
File        : FS_Warn.c
Purpose     : Logging (used only at higher debug levels)
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/
#include <stdio.h>
#include <string.h>
#include "FS_Int.h"
#include "FS_Debug.h"
#include "FS_CLib.h"

/*********************************************************************
*
*       Defines
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
*  Note: These routines are needed only in higher debug levels.
*
**********************************************************************
*/
/*********************************************************************
*
*       FS_Warn
*/
void FS__Warn(const char *s) {
  FS_X_Warn(s);
}

/*********************************************************************
*
*       FS__Warn1
*/
void FS__Warn1(const char *s, int p0) {
  char ac[MAXLEN + 10];
  char* sOut = ac;
  _CopyString(ac, s, MAXLEN);
  sOut += FS_STRLEN(sOut);
  FS__AddSpaceHex(p0, 8, &sOut);
  FS__Warn(ac);
}

/*********************************************************************
*
*       FS__Warn2
*/
void FS__Warn2(const char *s, int p0, int p1) {
  char ac[MAXLEN + 20];
  char* sOut = ac;
  _CopyString(ac, s, MAXLEN);
  sOut += FS_STRLEN(sOut);
  FS__AddSpaceHex(p0, 8, &sOut);
  FS__AddSpaceHex(p1, 8, &sOut);
  FS__Warn(ac);
}

/*********************************************************************
*
*       FS__Warn3
*/
void FS__Warn3(const char *s, int p0, int p1, int p2) {
  char ac[MAXLEN + 30];
  char* sOut = ac;
  _CopyString(ac, s, MAXLEN);
  sOut += FS_STRLEN(sOut);
  FS__AddSpaceHex(p0, 8, &sOut);
  FS__AddSpaceHex(p1, 8, &sOut);
  FS__AddSpaceHex(p2, 8, &sOut);
  FS__Warn(ac);
}

/*********************************************************************
*
*       FS_Warn4
*/
void FS__Warn4(const char *s, int p0, int p1, int p2, int p3) {
  char ac[MAXLEN + 40];
  char* sOut = ac;
  _CopyString(ac, s, MAXLEN);
  sOut += FS_STRLEN(sOut);
  FS__AddSpaceHex(p0, 8, &sOut);
  FS__AddSpaceHex(p1, 8, &sOut);
  FS__AddSpaceHex(p2, 8, &sOut);
  FS__AddSpaceHex(p3, 8, &sOut);
  FS__Warn(ac);
}

/*************************** End of file ****************************/
