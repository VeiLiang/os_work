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
File        : MMC_X_HW.h
Purpose     : MMC hardware layer
---------------------------END-OF-HEADER------------------------------
*/

#ifndef __MMC_X_HW_H__
#define __MMC_X_HW_H__

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
void  FS_MMC_HW_X_EnableCS   (U8 Unit);
void  FS_MMC_HW_X_DisableCS  (U8 Unit);

/* Medium status functions */
int   FS_MMC_HW_X_IsPresent        (U8 Unit);
int   FS_MMC_HW_X_IsWriteProtected (U8 Unit);

/* Operation condition detection & adjusting */
U16   FS_MMC_HW_X_SetMaxSpeed(U8 Unit, U16 MaxFreq);
int   FS_MMC_HW_X_SetVoltage (U8 Unit, U16 Vmin,   U16 Vmax);

/* Data transfer functions */
void  FS_MMC_HW_X_Read (U8 Unit,       U8 * pData, int NumBytes);
void  FS_MMC_HW_X_Write(U8 Unit, const U8 * pData, int NumBytes);

#ifdef __cplusplus
}
#endif

#endif  /* __MMC_X_HW_H__ */

/*************************** End of file ****************************/
