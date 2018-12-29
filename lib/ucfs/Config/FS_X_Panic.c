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
File        : FS_X_Panic.c
Purpose     : Panic routine.
              Referred in debug builds of the file system only and
              called only in case of fatal, unrecoverable errors.
---------------------------END-OF-HEADER------------------------------
*/
#include "FS.h"
#include "xm_printf.h"
/*********************************************************************
*
*       Public code
*
**********************************************************************
*/


/*********************************************************************
*
*       FS_X_Panic
*/
void FS_X_Panic(int ErrorCode) {
  XM_printf ("FS_X_Panic ErrorCode=%d\n", ErrorCode);
  while (1);
}

/*************************** End of file ****************************/
