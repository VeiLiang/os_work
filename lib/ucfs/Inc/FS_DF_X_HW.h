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
File        : DataFlash_X_HW.h
Purpose     : Data Flash hardware layer
---------------------------END-OF-HEADER------------------------------
*/

#ifndef __DATAFLASH_X_HW_H__
#define __DATAFLASH_X_HW_H__

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
*
*       Global function prototypes
*
**********************************************************************
*/

/* Control line functions */
void          FS_DF_HW_X_EnableCS   (U8 Unit);
void          FS_DF_HW_X_DisableCS  (U8 Unit);
int           FS_DF_HW_X_Init       (U8 Unit);

/* Data transfer functions */
void          FS_DF_HW_X_Read (U8 Unit,       U8 * pData, int NumBytes);
void          FS_DF_HW_X_Write(U8 Unit, const U8 * pData, int NumBytes);

#ifdef __cplusplus
}
#endif

#endif  /* __DATAFLASH_X_HW_H__ */

/*************************** End of file ****************************/
