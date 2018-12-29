/***********************************************************************
Copyright (c)2007 Arkmicro Technologies Inc.  All Rights Reserved
Filename: types.h
Version : 1.0
Date    : 2007.09
Author  : Harey
Abstract: types definition for ark1900
History :
************************************************************************/
#ifndef _TYPES_H_
#define _TYPES_H_

#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef TRUE
#define TRUE    (!0)
#endif

#define NO      0
#define YES     1

#define OFF     0
#define ON      1

#define VALUE_DEC     0
#define VALUE_INC      1

#ifndef NULL
#define NULL    0
#endif

#define PI 3.1415926

#include "bits.h"
#include "xm_type.h"


typedef int                 BOOL;
typedef unsigned char  INT8U;                    /* Unsigned  8 bit quantity                           */
typedef signed   char  INT8S;                    /* Signed    8 bit quantity                           */
typedef unsigned short INT16U;                   /* Unsigned 16 bit quantity                           */
typedef signed   short INT16S;                   /* Signed   16 bit quantity                           */
typedef unsigned long  INT32U;                   /* Unsigned 32 bit quantity                           */
typedef signed   long  INT32S;                   /* Signed   32 bit quantity                           */
typedef float          FP32;                     /* Single precision floating point                    */
typedef double         FP64;                     /* Double precision floating point                    */

#endif // _TYPES_H_

