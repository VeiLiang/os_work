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

#ifndef __MUSB_UCOS_BOARD_ARCH_H__
#define __MUSB_UCOS_BOARD_ARCH_H__

/*
 * AFS-specific board architecture macros
 * $Revision: 1.1 $
 */

#include "mu_tools.h"

#if MUSB_DIAG >= 3

#undef MGC_Read8
extern uint8_t MGC_UcosRead8(uint8_t* pBase, uint16_t wOffset);
#define MGC_Read8(_pBase, _offset) MGC_UcosRead8(_pBase, _offset)

#undef MGC_Read16
extern uint16_t MGC_UcosRead16(uint8_t* pBase, uint16_t wOffset);
#define MGC_Read16(_pBase, _offset) MGC_UcosRead16(_pBase, _offset)

#undef MGC_Read32
extern uint32_t MGC_UcosRead32(uint8_t* pBase, uint16_t wOffset);
#define MGC_Read32(_pBase, _offset) MGC_UcosRead32(_pBase, _offset)

#undef MGC_Write8
extern void MGC_UcosWrite8(uint8_t* pBase, uint16_t wOffset, uint8_t bDatum);
#define MGC_Write8(_pBase, _offset, _data) MGC_UcosWrite8(_pBase, _offset, _data)

#undef MGC_Write16
extern void MGC_UcosWrite16(uint8_t* pBase, uint16_t wOffset, uint16_t wDatum);
#define MGC_Write16(_pBase, _offset, _data) MGC_UcosWrite16(_pBase, _offset, _data)

#undef MGC_Write32
extern void MGC_UcosWrite32(uint8_t* pBase, uint16_t wOffset, uint32_t dwDatum);
#define MGC_Write32(_pBase, _offset, _data) MGC_UcosWrite32(_pBase, _offset, _data)

#endif

#endif	/* multiple inclusion protection */
