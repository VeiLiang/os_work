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

/*
 * MUSBStack-S memory abstraction.
 * $Revision: 1.1 $
 */

#ifndef __MUSB_MEMORY_H__
#define __MUSB_MEMORY_H__

#include "mu_tools.h"

#ifdef MUSB_STDLIB

/* Just use stdlib */

#include <stdlib.h>
#include <string.h>

#define MUSB_MemAlloc Dmpmalloc
#define MUSB_MemRealloc Dmprealloc
#define MUSB_MemFree Dmpfree
#define MUSB_MemCopy memcpy
#define MUSB_MemSet memset

#else 

/* Allow platform to define */
#include "plat_mem.h"

/* Be sure we have something for each one */

#ifndef MUSB_MemAlloc
#define MUSB_MemAlloc malloc
#endif

#ifndef MUSB_MemRealloc
#define MUSB_MemRealloc realloc
#endif

#ifndef MUSB_MemFree
#define MUSB_MemFree free
#endif

#ifndef MUSB_MemCopy
#define MUSB_MemCopy memcpy
#endif

#ifndef MUSB_MemSet
#define MUSB_MemSet memset
#endif

#endif  /* endif for MUSB_STDLIB */

#endif	/* multiple inclusion protection */
