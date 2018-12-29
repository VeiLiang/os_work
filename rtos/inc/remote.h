/*
**********************************************************************
Copyright (c)2007 Arkmicro Technologies Inc.  All Rights Reserved
Filename: remote.c
Version : 1.0
Date    : 2007.07.29
Author  : Harey
Abstract: Header file for ark1600 remote module
History :
***********************************************************************
*/

#ifndef _REMOTE_H_
#define _REMOTE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG_REMOTE	0

//#define REMOTE_CODE_ARK9
//#define REMOTE_CODE_CHIP
//#define REMOTE_CODE_SOUND
//#define remote_mode
#define REMOTE_CODE_TACHOGRAPH	

typedef struct
{
	unsigned char value;
	unsigned char repeate;
	unsigned char release;
} REMOTE_DATA;


#define REMOTE_NULL        0xFF     //ÎÞÐ§Ò£¿ØÂë

// Ò£¿ØÆ÷Ä£¿é¿ªÆô
void RemoteKeyInit (void);

// Ò£¿ØÆ÷Ä£¿é¹Ø±Õ
void RemoteKeyExit (void);

/*********************************************************************
	Set the report interval for remote repeating event
Parameter:
	ulMillisecond: interval, unit as millisecond, this parameter should be the multiply of
	 10 millsecond.
*********************************************************************/
void SetRemoteKeyRepeateInterval(UINT32 ulMillisecond);

/*********************************************************************
	Get the report interval for remote repeating event
Return:
	millisendonds for interval
*********************************************************************/
UINT32 GetRemoteKeyRepeatInterval(void);

#ifdef __cplusplus
}
#endif

#endif /* _REMOTE_H_ */

