/******************************************************************
 *                                                                *
 *        Copyright Mentor Graphics Corporation 2005              *
 *                                                                *
 *                All Rights Reserved.                            *
 *                                                                *
 *    THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION *
 *  WHICH IS THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS   *
 *  LICENSORS AND IS SUBJECT TO LICENSE TERMS.                    *
 *                                                                *
 ******************************************************************/

#ifndef __MUSB_UCOS_BOARD_H__
#define __MUSB_UCOS_BOARD_H__

/*
 * AFS-specific controller list
 * $Revision: 1.1 $
 */
#include "plat_cnf.h"
#include "mu_dsi.h"
#include "irqs.h"

MUSB_UcosController MUSB_aUcosController[] =
{
#ifdef MUSB_HDRC
    { MUSB_CONTROLLER_HDRC, (void*)USB_BASE, USB_INT, TRUE },
#endif
#ifdef MUSB_MHDRC
    { MUSB_CONTROLLER_MHDRC, (void*)USB_BASE, USB_INT, TRUE },
#endif
#ifdef MUSB_HSFC
    { MUSB_CONTROLLER_HSFC, (void*)USB_BASE, USB_INT, TRUE },
#endif
#ifdef MUSB_FDRC
    { MUSB_CONTROLLER_FDRC, (void*)USB_BASE, USB_INT, FALSE },
#endif
};

#endif	/* multiple inclusion protection */
