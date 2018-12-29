///////////////////////////////////////////////////////////////////////////////
// Uart_Protocal.h: 
// Created By Winton.Wang 2016-12-27

///////////////////////////////////////////////////////////////////////////////
#ifndef _UART_PROTOCAL_H_
#define _UART_PROTOCAL_H_
//////////////////////////////////////////////////////////////////////
#ifdef  __cplusplus
extern "C" {
#endif  // __cplusplus

typedef unsigned char	u8;
typedef unsigned short	u16;
typedef unsigned int	u32;

//#define FALSE 0
//#define TRUE !FALSE

#define 	GPS_DATA_SIZE			(512)
#define	DATA_OFFSET			(4)

typedef struct
{
	u16 m_dwWrId;				
	u16 m_dwRdId;									
}FIFO_BUF_STRUCT;
typedef struct
{
	u8 m_bRMCYear;					//
	u8 m_btRMCMonth;				//
	u8 m_btRMCDay;					//
	u8 m_btRMCUTCHour;			//保存的UTC时间
	u8 m_btRMCMinute;				//
	u8 m_btRMCSecond;				//
}GPS_TIME;

void GPS_ReceiveOneByte (unsigned char temp);
void GPS_DecodeUart(void);
void GPS_Init (void);


//////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif          // __cplusplus

#endif // _NMEAPARSER_H_
