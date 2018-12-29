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
File        : IDE_X_HW.h
Purpose     : IDE hardware layer
----------------------------------------------------------------------
Known problems or limitations with current version
----------------------------------------------------------------------
None.
---------------------------END-OF-HEADER------------------------------
*/

#ifndef __IDE_X_HW_H__
#define __IDE_X_HW_H__

#include "Global.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
*
*             Global function prototypes
*
**********************************************************************
*/

/* Control line functions */
void FS_IDE_HW_Reset     (U8 Unit);
int  FS_IDE_HW_IsPresent (U8 Unit);
void FS_IDE_HW_Delay400ns(U8 Unit);

U16  FS_IDE_HW_ReadReg  (U8 Unit, unsigned AddrOff);
void FS_IDE_HW_ReadData (U8 Unit,       U8 * pData, unsigned NumBytes);
void FS_IDE_HW_WriteData(U8 Unit, const U8 * pData, unsigned NumBytes);
void FS_IDE_HW_WriteReg (U8 Unit, unsigned AddrOff, U16 Data);
/* Status detection functions */

#ifdef __cplusplus
}
#endif

#endif  /* __IDE_X_HW_H__ */

/*************************** End of file ****************************/
