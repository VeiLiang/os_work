/******************************************************************
 *                                                                *
 *        Copyright Mentor Graphics Corporation 2004              *
 *                                                                *
 *                All Rights Reserved.                            *
 *                                                                *
 *    THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION *
 *  WHICH IS THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS   *
 *  LICENSORS AND IS SUBJECT TO LICENSE TERMS.                    *
 *                                                                *
 ******************************************************************/

#ifndef __MUSB_UCOS_AFS_MEMORY_H__
#define __MUSB_UCOS_AFS_MEMORY_H__

/*
 * uC/OS/AFS-specific memory abstraction
 * $Revision: 1.1 $
 */

#include "DmpMemory.h"

#define MUSB_MemAlloc 		Dmpmalloc
#define MUSB_MemRealloc 		MGC_AfsMemRealloc
#define MUSB_MemFree 		Dmpfree

#define MUSB_MemCopy(_pDest, _pSrc, _iSize) \
    memcpy((void*)_pDest, (void*)_pSrc, _iSize)
#define MUSB_MemSet(_pDest, _iData, _iSize) \
    memset((void*)_pDest, _iData, _iSize)

//extern void* MGC_AfsMemRealloc(void* pBuffer, uint32_t iSize);
extern void* MGC_AfsMemRealloc(void* pBlock, size_t iSize);
#endif	/* multiple inclusion protection */
