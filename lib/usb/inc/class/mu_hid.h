/*****************************************************************************
 *                                                                           *
 *      Copyright Mentor Graphics Corporation 2003-2004                      *
 *                                                                           *
 *                All Rights Reserved.                                       *
 *                                                                           *
 *    THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION            *
 *  WHICH IS THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS              *
 *  LICENSORS AND IS SUBJECT TO LICENSE TERMS.                               *
 *                                                                           *
 ****************************************************************************/

/*
 * MUSBStack-S HID (Human Interface Device) definitions.
 * $Revision: 1.1 $
 */

#ifndef __MUSB_HID_H__
#define __MUSB_HID_H__

#include "mu_dev.h"
#include "mu_tools.h"

#define MUSB_DT_HID				(MUSB_TYPE_CLASS | 0x01)
#define MUSB_DT_REPORT			(MUSB_TYPE_CLASS | 0x02)
#define MUSB_DT_PHYSICAL			(MUSB_TYPE_CLASS | 0x03)
#define MUSB_DT_HUB				(MUSB_TYPE_CLASS | 0x09)

#define MUSB_DT_HID_SIZE			9

/*
* HID Class Descriptor Types
*/

#define MUSB_HID_DESCRIPTOR			0x21
#define MUSB_REPORT_DESCRIPTOR		0x22
#define MUSB_PHYSICAL_DESCRIPTOR	0x23

/*
* HID requests
*/

#define MUSB_REQ_GET_REPORT			0x01
#define MUSB_REQ_GET_IDLE			0x02
#define MUSB_REQ_GET_PROTOCOL		0x03
#define MUSB_REQ_SET_REPORT			0x09
#define MUSB_REQ_SET_IDLE			0x0A
#define MUSB_REQ_SET_PROTOCOL		0x0B

/*
*  HID Report Types
*/

#define MUSB_HID_INPUT_REPORT		0x01
#define MUSB_HID_OUTPUT_REPORT		0x02
#define MUSB_HID_FEATURE_REPORT		0x03

/* HID descriptor */
#include "mu_pkon.h"
typedef	struct
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdHID;
    uint8_t bCountryCode;
    uint8_t bNumDescriptors;
    uint8_t bClassDescriptorType;
    uint16_t wDescriptorLength;
} MUSB_HidDescriptor;
#include "mu_pkoff.h"

/* HID optional descriptor */
#include "mu_pkon.h"
typedef	struct
{
    uint8_t bDescriptorType;
    uint16_t wDescriptorLength;
} MUSB_HidOptionalDescriptor;
#include "mu_pkoff.h"

#endif	/* multiple inclusion protection */
