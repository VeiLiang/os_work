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

/*
 * MUSBStack-S IPOD Protocol/Command-Set Interface
 * $Revision: 1.1 $
 */

#ifndef __MUSB_IPOD_CTRL_H__
#define __MUSB_IPOD_CTRL_H__

#include "mu_cdi.h"
#include "mu_iapi.h"

#define REGISTER_USER_FUNC				0x00
#define REQUEST_CONTROL_IPOD			0x01

#define SMALL_TELGRAM_MAXLEN			253

#define SYNC_BYTE						0xFF
#define START_TELEGRAM					0x55

typedef struct _IPODHID_LONG_TELEGRAM_ {
    UINT8 tlmstart;
    UINT8 markerlen;
    UINT8 lengthH;
    UINT8 lengthL;
    UINT8 lignoID;
    UINT8 *ptlmData;
}IPODHID_LONG_TELEGRAM;


typedef struct _IPODHID_SMALL_TELEGRAM_ {
    UINT8 tlmstart;
    UINT8 tlmlength;
    UINT8 lignoID;
    UINT8 *ptlmData;
}IPODHID_SMALL_TELEGRAM;


typedef struct _IPODIAP_PACKET_{
    UINT8 reportID;
    UINT8 multipacket;
    union
    {
        IPODHID_SMALL_TELEGRAM  smallTlm;
        IPODHID_LONG_TELEGRAM   longTlm;
    }telegram;
}IPODIAP_PACKET;


typedef struct _IPODIAP_COMM_RETPACKET_
{
    UINT8 reportID;
    UINT8 multipacket;
    UINT8 tlmstart;
    UINT8 tlmlength;
    UINT8 lignoID;
    UINT8 commandID;
    UINT8 commandData[256];  //last data is checksum
}IPODIAP_COMM_RETPACKET;

typedef struct _IPODIAP_EXT_RETPACKET_
{
    UINT8 reportID;
    UINT8 multipacket;
    UINT8 tlmstart;
    UINT8 tlmlength;
    UINT8 lignoID;
    UINT8 commandIDH;
    UINT8 commandIDL;
    UINT8 commandData[253]; //last data is checksum
}IPODIAP_EXT_RETPACKET;

typedef struct _IPOD_RETPACKET_
{
    union
    {
        IPODIAP_COMM_RETPACKET comm_retpacket;
        IPODIAP_EXT_RETPACKET  ext_retpacket;
        unsigned char data[300];
    }packetunit;
}IPOD_RETPACKET;


uint32_t MUSB_IPOD_Control(uint16_t Command, void *Arg);
uint32_t MGC_Fill_IPODIAP_SmallPacket(IPODIAP_PACKET *pIAPPacket,unsigned char reportID,
	unsigned char mult, unsigned char lingoID, unsigned char smalllen, unsigned char *pdata);
uint32_t MGC_Fill_IPODIAP_LongPacket(IPODIAP_PACKET *pIAPPacket,unsigned char reportID,
	unsigned char mult, unsigned char lingoID, unsigned int longlen, unsigned char *pdata);

#endif	/* multiple inclusion protection */

