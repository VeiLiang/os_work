/***********************************************************************
*Copyright shenzhen exceedspace company
*All Rights Reserved 
*
*Filename:    crc16.c
*Version :    
*Date    :    
*Author  :    
*Abstract:    
*History :     oliver add/edit
************************************************************************/
 

#ifndef _CRC16_H_
#define _CRC16_H_


#if defined (__cplusplus)
	extern "C"{
#endif

unsigned short crc16_ccitt(const char *buf, int len);
unsigned char checksum_byte(const char *buf, int len);
unsigned int checksum_int(const unsigned int *buf, int len);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */


#endif /* _CRC16_H_ */
