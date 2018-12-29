/*
**********************************************************************
Copyright (c)2007 Arkmicro Technologies Inc.  All Rights Reserved 
Filename: fs_card.c
Version : 1.0
Date    : 2007.07.24
Author  : Harey
Abstract: fs card struct,which mainly used by card hotplug
History :
***********************************************************************
*/

#ifndef _FS_CARD_H_
#define _FS_CARD_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	unsigned char			Present;    //1 - card is in the socket
	unsigned char			Changed;    //1 - card changed(card is in socket)
	unsigned int			REG_CID[4];
	unsigned int			SecSize;    // the whole setctor size
	unsigned int			SecNum;     // the whole setctor number
} FS_CARD;

#ifdef __cplusplus
}
#endif

#endif /* _FS_CARD_H_ */

