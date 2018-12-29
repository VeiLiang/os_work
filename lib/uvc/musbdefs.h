
#ifndef __MUSB_MUSBDEFS_H__
#define __MUSB_MUSBDEFS_H__

#include "hardware.h"

typedef signed char s8;
typedef unsigned char uint8_t;
//typedef uint8_t u8;

typedef signed short s16;
typedef unsigned short uint16_t;
//typedef uint16_t u16;

typedef signed int s32;
typedef unsigned int uint32_t;
//typedef uint32_t u32;


/* Board-specific definitions (hard-wired controller locations/IRQs) */
#define MGC_MAX_USB_ENDS       16

#define MGC_END0_FIFOSIZE      64      /* this is non-configurable */

/*
 *     MUSBMHDRC Register map
 */

/* Common USB registers */
#define MGC_O_HDRC_FADDR	0x00	/* 8-bit */
#define MGC_O_HDRC_POWER	0x01	/* 8-bit */

#define MGC_O_HDRC_INTRTX	0x02	/* 16-bit */
#define MGC_O_HDRC_INTRRX       0x04
#define MGC_O_HDRC_INTRTXE      0x06
#define MGC_O_HDRC_INTRRXE      0x08
#define MGC_O_HDRC_INTRUSB      0x0A   /* 8 bit */
#define MGC_O_HDRC_INTRUSBE     0x0B   /* 8 bit */
#define MGC_O_HDRC_FRAME        0x0C
#define MGC_O_HDRC_INDEX        0x0E   /* 8 bit */
#define MGC_O_HDRC_TESTMODE     0x0F   /* 8 bit */


#define MUSB_FADDR	0x00	/* 8-BYTE */
#define MUSB_POWER	0x01	/* 8-BYTE */

#define MUSB_INTRTX	     0x02	/* 16-BYTE */
#define MUSB_INTRRX       0x04
#define MUSB_INTRTXE      0x06
#define MUSB_INTRRXE      0x08
#define MUSB_INTRUSB      0x0A   /* 8 BYTE */
#define MUSB_INTRUSBE     0x0B   /* 8 BYTE */
#define MUSB_FRAME        0x0C    /* 16 BIT */
#define MUSB_INDEX         0x0E   /* 8 BYTE */
#define MUSB_TESTMODE     0x0F   /* 8 BYTE */

#define	MUSB_DEVCTL	0x60	   /* 8 BYTE */
#define	MUSB_MISC   	0x61	   /* 8 BYTE */


#define MUSB_IDX_CSR0L        0x12 /*CSR0L is INDEX=0*/
#define MUSB_IDX_CSR0H        0x13 /*CSR0L is INDEX=0*/

#define MUSB_IDX_TXMAXP       0x10 /* except EP0 */
#define MUSB_IDX_TXCSRL        0x12 /*CSR0L is INDEX=0*/
#define MUSB_IDX_TXCSRH        0x13
#define MUSB_IDX_RXMAXP        0x14
#define MUSB_IDX_RXCSRL        0x16
#define MUSB_IDX_RXCSRH        0x17
#define MUSB_IDX_RXCOUNT     0x18
#define MUSB_IDX_NAKLIMIT0   0x1b
#define MUSB_FIFO_OFFSET(_bEnd) (0x20 + (_bEnd * 4))


#define  CSR0L_RxPktRdy            0x01
#define  CSR0L_TxPktRdy             0x02
#define  CSR0L_SentStall              0x04
#define  CSR0L_DataEnd               0x08
#define  CSR0L_SetupEnd             0x10
#define  CSR0L_SendStall                       0x20
#define  CSR0L_ServicedRxPktRdy         0x40
#define  CSR0L_ServicedSetupEnd         0x80

#define  CSR0H_FlushFIFO            0x01


#define  TXCSRL_TxPktRdy            0x01
#define  TXCSRL_FIFONotEmpty    0x02
#define  TXCSRL_UnderRun            0x04
#define  TXCSRL_FlushFIFO           0x08
#define  TXCSRL_SendStall            0x10
#define  TXCSRL_SentStall             0x20
#define  TXCSRL_ClrDataTog         0x40
#define  TXCSRL_IncompTx           0x80

#define  TXCSRH_DMAReqMode            0x04
#define  TXCSRH_FrcDataTog               0x08
#define  TXCSRH_DMAReqEnab             0x10
#define  TXCSRH_Mode                          0x20
#define  TXCSRH_ISO                             0x40
#define  TXCSRH_AutoSet                      0x80

#define  RXCSRL_RxPktRdy            0x01
#define  RXCSRL_FIFOFull              0x02
#define  RXCSRL_OverRun              0x04
#define  RXCSRL_DataError            0x08
#define  RXCSRL_FlushFIFO           0x10
#define  RXCSRL_SendStall             0x20
#define  RXCSRL_SentStall             0x40
#define  RXCSRL_ClrDataTog          0x80


#define  RXCSRH_IncompRx            0x01
#define  RXCSRH_DMAReqMode       0x08
#define  RXCSRH_PIDError              0x10
#define  RXCSRH_DMAReqEnab       0x20
#define  RXCSRH_ISO                      0x40
#define  RXCSRH_AutoClr                0x80



/* Get offset for a given FIFO */
#define MGC_FIFO_OFFSET(_bEnd) (0x20 + (_bEnd * 4))

/* Additional Control Registers */

#define	MGC_O_HDRC_DEVCTL	0x60	   /* 8 bit */

/* These are actually indexed: */
#define MGC_O_HDRC_TXFIFOSZ	0x62	/* 8-BYTE (see masks) */
#define MGC_O_HDRC_RXFIFOSZ	0x63	/* 8-BYTE (see masks) */
#define MGC_O_HDRC_TXFIFOADD	0x64	/* 16-BYTE offset shifted right 3 */
#define MGC_O_HDRC_RXFIFOADD	0x66	/* 16-BYTE offset shifted right 3 */

/* offsets to registers in flat model */
#define MGC_O_HDRC_TXMAXP	0x00
#define MGC_O_HDRC_TXCSR	0x02
#define MGC_O_HDRC_CSR0	MGC_O_HDRC_TXCSR	/* re-used for EP0 */
#define MGC_O_HDRC_RXMAXP	0x04
#define MGC_O_HDRC_RXCSR	0x06
#define MGC_O_HDRC_RXCOUNT	0x08
#define MGC_O_HDRC_COUNT0	MGC_O_HDRC_RXCOUNT	/* re-used for EP0 */
#define MGC_O_HDRC_TXTYPE	0x0A
#define MGC_O_HDRC_TYPE0	MGC_O_HDRC_TXTYPE	/* re-used for EP0 */
#define MGC_O_HDRC_TXINTERVAL	0x0B
#define MGC_O_HDRC_NAKLIMIT0	MGC_O_HDRC_TXINTERVAL	/* re-used for EP0 */
#define MGC_O_HDRC_RXTYPE	0x0C
#define MGC_O_HDRC_RXINTERVAL	0x0D
#define MGC_O_HDRC_FIFOSIZE	0x0F
#define MGC_O_HDRC_CONFIGDATA	MGC_O_HDRC_FIFOSIZE	/* re-used for EP0 */

#define MGC_END_OFFSET(_bEnd, _bOffset)	(0x100 + (0x10*_bEnd) + _bOffset)

/* "bus control" registers */
#define MGC_O_HDRC_TXFUNCADDR	0x00
#define MGC_O_HDRC_TXHUBADDR	0x02
#define MGC_O_HDRC_TXHUBPORT	0x03

#define MGC_O_HDRC_RXFUNCADDR	0x04
#define MGC_O_HDRC_RXHUBADDR	0x06
#define MGC_O_HDRC_RXHUBPORT	0x07

#define MGC_BUSCTL_OFFSET(_bEnd, _bOffset)	(0x80 + (8*_bEnd) + _bOffset)

/*
 *     MUSBHDRC Register BYTE masks
 */

/* POWER */

#define MGC_M_POWER_ISOUPDATE   0x80
#define	MGC_M_POWER_SOFTCONN    0x40
#define	MGC_M_POWER_HSENAB	0x20
#define	MGC_M_POWER_HSMODE	0x10
#define MGC_M_POWER_RESET       0x08
#define MGC_M_POWER_RESUME      0x04
#define MGC_M_POWER_SUSPENDM    0x02
#define MGC_M_POWER_ENSUSPEND   0x01

/* INTRUSB */
#define MGC_M_INTR_SUSPEND    0x01
#define MGC_M_INTR_RESUME     0x02
#define MGC_M_INTR_RESET      0x04
#define MGC_M_INTR_BABBLE     0x04
#define MGC_M_INTR_SOF        0x08
#define MGC_M_INTR_CONNECT    0x10
#define MGC_M_INTR_DISCONNECT 0x20
#define MGC_M_INTR_SESSREQ    0x40
#define MGC_M_INTR_VBUSERROR  0x80   /* FOR SESSION END */
#define MGC_M_INTR_EP0      0x01  /* FOR EP0 INTERRUPT */

/* DEVCTL */
#define MGC_M_DEVCTL_BDEVICE    0x80
#define MGC_M_DEVCTL_FSDEV      0x40
#define MGC_M_DEVCTL_LSDEV      0x20
#define MGC_M_DEVCTL_VBUS       0x18
#define MGC_S_DEVCTL_VBUS       3
#define MGC_M_DEVCTL_HM         0x04
#define MGC_M_DEVCTL_HR         0x02
#define MGC_M_DEVCTL_SESSION    0x01

/* TESTMODE */

#define MGC_M_TEST_FORCE_HOST   0x80
#define MGC_M_TEST_FIFO_ACCESS  0x40
#define MGC_M_TEST_FORCE_FS     0x20
#define MGC_M_TEST_FORCE_HS     0x10
#define MGC_M_TEST_PACKET       0x08
#define MGC_M_TEST_K            0x04
#define MGC_M_TEST_J            0x02
#define MGC_M_TEST_SE0_NAK      0x01

/* allocate for double-packet buffering (effectively doubles assigned _SIZE) */
#define MGC_M_FIFOSZ_DPB	0x10
/* allocation size (8, 16, 32, ... 4096) */
#define MGC_M_FIFOSZ_SIZE	0x0f

/* CSR0 */
#define	MGC_M_CSR0_FLUSHFIFO      0x0100
#define MGC_M_CSR0_TXPKTRDY       0x0002
#define MGC_M_CSR0_RXPKTRDY       0x0001

/* CSR0 in Peripheral mode */
#define MGC_M_CSR0_P_SVDSETUPEND  0x0080
#define MGC_M_CSR0_P_SVDRXPKTRDY  0x0040
#define MGC_M_CSR0_P_SENDSTALL    0x0020
#define MGC_M_CSR0_P_SETUPEND     0x0010
#define MGC_M_CSR0_P_DATAEND      0x0008
#define MGC_M_CSR0_P_SENTSTALL    0x0004

/* CSR0 in Host mode */
#define MGC_M_CSR0_H_NO_PING	  0x0800
#define MGC_M_CSR0_H_WR_DATATOGGLE   0x0400	/* set to allow setting: */
#define MGC_M_CSR0_H_DATATOGGLE	    0x0200	/* data toggle control */
#define	MGC_M_CSR0_H_NAKTIMEOUT   0x0080
#define MGC_M_CSR0_H_STATUSPKT    0x0040
#define MGC_M_CSR0_H_REQPKT       0x0020
#define MGC_M_CSR0_H_ERROR        0x0010
#define MGC_M_CSR0_H_SETUPPKT     0x0008
#define MGC_M_CSR0_H_RXSTALL      0x0004

/* TxType/RxType */
#define MGC_M_TYPE_SPEED	0xc0
#define MGC_S_TYPE_SPEED	6
#define MGC_TYPE_SPEED_HIGH	1
#define MGC_TYPE_SPEED_FULL	2
#define MGC_TYPE_SPEED_LOW	3
#define MGC_M_TYPE_PROTO	0x30
#define MGC_S_TYPE_PROTO	4
#define MGC_M_TYPE_REMOTE_END	0xf

/* CONFIGDATA */

#define MGC_M_CONFIGDATA_MPRXE      0x80	/* auto bulk pkt combining */
#define MGC_M_CONFIGDATA_MPTXE      0x40	/* auto bulk pkt splitting */
#define MGC_M_CONFIGDATA_BIGENDIAN  0x20
#define MGC_M_CONFIGDATA_HBRXE      0x10	/* HB-ISO for RX */
#define MGC_M_CONFIGDATA_HBTXE      0x08	/* HB-ISO for TX */
#define MGC_M_CONFIGDATA_DYNFIFO    0x04	/* dynamic FIFO sizing */
#define MGC_M_CONFIGDATA_SOFTCONE   0x02	/* SoftConnect */
#define MGC_M_CONFIGDATA_UTMIDW     0x01   /* data width 0 => 8bits, 1 => 16bits */

/* TXCSR in Peripheral and Host mode */

#define MGC_M_TXCSR_AUTOSET       0x8000
#define MGC_M_TXCSR_ISO           0x4000
#define MGC_M_TXCSR_MODE          0x2000
#define MGC_M_TXCSR_DMAENAB       0x1000
#define MGC_M_TXCSR_FRCDATATOG    0x0800
#define MGC_M_TXCSR_DMAMODE       0x0400
#define MGC_M_TXCSR_CLRDATATOG    0x0040
#define MGC_M_TXCSR_FLUSHFIFO     0x0008
#define MGC_M_TXCSR_FIFONOTEMPTY  0x0002
#define MGC_M_TXCSR_TXPKTRDY      0x0001

/* TXCSR in Peripheral mode */

#define MGC_M_TXCSR_P_INCOMPTX    0x0080
#define MGC_M_TXCSR_P_SENTSTALL   0x0020
#define MGC_M_TXCSR_P_SENDSTALL   0x0010
#define MGC_M_TXCSR_P_UNDERRUN    0x0004

/* TXCSR in Host mode */

#define MGC_M_TXCSR_H_WR_DATATOGGLE   0x0200
#define MGC_M_TXCSR_H_DATATOGGLE      0x0100
#define MGC_M_TXCSR_H_NAKTIMEOUT  0x0080
#define MGC_M_TXCSR_H_RXSTALL     0x0020
#define MGC_M_TXCSR_H_ERROR       0x0004

/* RXCSR in Peripheral and Host mode */

#define MGC_M_RXCSR_AUTOCLEAR     0x8000
#define MGC_M_RXCSR_DMAENAB       0x2000
#define MGC_M_RXCSR_DISNYET       0x1000
#define MGC_M_RXCSR_DMAMODE       0x0800
#define MGC_M_RXCSR_INCOMPRX      0x0100
#define MGC_M_RXCSR_CLRDATATOG    0x0080
#define MGC_M_RXCSR_FLUSHFIFO     0x0010
#define MGC_M_RXCSR_DATAERROR     0x0008
#define MGC_M_RXCSR_FIFOFULL      0x0002
#define MGC_M_RXCSR_RXPKTRDY      0x0001

/* RXCSR in Peripheral mode */

#define MGC_M_RXCSR_P_ISO         0x4000
#define MGC_M_RXCSR_P_SENTSTALL   0x0040
#define MGC_M_RXCSR_P_SENDSTALL   0x0020
#define MGC_M_RXCSR_P_OVERRUN     0x0004

/* RXCSR in Host mode */

#define MGC_M_RXCSR_H_AUTOREQ     0x4000
#define MGC_M_RXCSR_H_WR_DATATOGGLE   0x0400
#define MGC_M_RXCSR_H_DATATOGGLE        0x0200
#define MGC_M_RXCSR_H_RXSTALL     0x0040
#define MGC_M_RXCSR_H_REQPKT      0x0020
#define MGC_M_RXCSR_H_ERROR       0x0004

/* HUBADDR */
#define MGC_M_HUBADDR_MULTI_TT		0x80


/* TXCSR in Peripheral and Host mode */

#define MGC_M_TXCSR2_AUTOSET       0x80
#define MGC_M_TXCSR2_ISO           0x40
#define MGC_M_TXCSR2_MODE          0x20
#define MGC_M_TXCSR2_DMAENAB       0x10
#define MGC_M_TXCSR2_FRCDATATOG    0x08
#define MGC_M_TXCSR2_DMAMODE       0x04

#define MGC_M_TXCSR1_CLRDATATOG    0x40
#define MGC_M_TXCSR1_FLUSHFIFO     0x08
#define MGC_M_TXCSR1_FIFONOTEMPTY  0x02
#define MGC_M_TXCSR1_TXPKTRDY      0x01

/* TXCSR in Peripheral mode */

#define MGC_M_TXCSR1_P_INCOMPTX    0x80
#define MGC_M_TXCSR1_P_SENTSTALL   0x20
#define MGC_M_TXCSR1_P_SENDSTALL   0x10
#define MGC_M_TXCSR1_P_UNDERRUN    0x04

/* TXCSR in Host mode */

#define MGC_M_TXCSR1_H_NAKTIMEOUT  0x80
#define MGC_M_TXCSR1_H_RXSTALL     0x20
#define MGC_M_TXCSR1_H_ERROR       0x04

/* RXCSR in Peripheral and Host mode */

#define MGC_M_RXCSR2_AUTOCLEAR     0x80
#define MGC_M_RXCSR2_DMAENAB       0x20
#define MGC_M_RXCSR2_DISNYET       0x10
#define MGC_M_RXCSR2_DMAMODE       0x08
#define MGC_M_RXCSR2_INCOMPRX      0x01

#define MGC_M_RXCSR1_CLRDATATOG    0x80
#define MGC_M_RXCSR1_FLUSHFIFO     0x10
#define MGC_M_RXCSR1_DATAERROR     0x08
#define MGC_M_RXCSR1_FIFOFULL      0x02
#define MGC_M_RXCSR1_RXPKTRDY      0x01

/* RXCSR in Peripheral mode */

#define MGC_M_RXCSR2_P_ISO         0x40
#define MGC_M_RXCSR1_P_SENTSTALL   0x40
#define MGC_M_RXCSR1_P_SENDSTALL   0x20
#define MGC_M_RXCSR1_P_OVERRUN     0x04

/* RXCSR in Host mode */

#define MGC_M_RXCSR2_H_AUTOREQ     0x40
#define MGC_M_RXCSR1_H_RXSTALL     0x40
#define MGC_M_RXCSR1_H_REQPKT      0x20
#define MGC_M_RXCSR1_H_ERROR       0x04

#ifndef NULL
#define NULL 0
#endif

#define min(a,b) ((a < b) ? a : b)

/*
 * Target-specific configuration declarations
 */

/** HDRC */
#define MUSB_CONTROLLER_HDRC            1

/** MHDRC */
#define MUSB_CONTROLLER_MHDRC           2

/****************************** VERIFY THE DEFINES **************************
 * determine how to compile the driver; MUSB_GADGET->as gadget driver,
 * MUSB_HOST as host mode, MUSB_OTG -> otg mode (host and gadget)
 *
 * OTG => GADGET
 */

#define MGC_Read8(_offset) *((volatile uint8_t*)((uint8_t *)USB_BASE + _offset))
#define MGC_Read16(_offset) *((volatile uint16_t*)((uint8_t *)USB_BASE + _offset))
#define MGC_Read32(_offset) *((volatile uint32_t*)((uint8_t *)USB_BASE + _offset))

#define MGC_Write8(_offset, _data) (*(volatile uint8_t*)((uint8_t *)USB_BASE + _offset) = _data)
#define MGC_Write16(_offset, _data) (*(volatile uint16_t*)((uint8_t *)USB_BASE + _offset) = _data)
#define MGC_Write32(_offset, _data) (*(volatile uint32_t*)((uint8_t *)USB_BASE + _offset) = _data)
/* Get offset for a given FIFO */
#define MGC_FIFO_OFFSET(_bEnd) (0x20 + (_bEnd * 4))

/****************************** USB CONSTANTS ********************************/

/****************************** DEBUG CONSTANTS ********************************/

#define MGC_TEST_PACKET_SIZE 53

/****************************** CONSTANTS ********************************/

#define STATIC static

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef MUSB_C_NUM_EPS
#define MUSB_C_NUM_EPS ((uint8_t)16)
#endif

#ifndef MUSB_MAX_END0_PACKET
#define MUSB_MAX_END0_PACKET ((uint16_t)MGC_END0_FIFOSIZE)
#endif

#define MGC_END0_START  0x0
#define MGC_END0_OUT    0x2
#define MGC_END0_IN     0x4
#define MGC_END0_STATUS 0x8

#define MGC_END0_STAGE_SETUP 		0x0
#define MGC_END0_STAGE_TX		0x2
#define MGC_END0_STAGE_RX		0x4
#define MGC_END0_STAGE_STATUSIN		0x8
#define MGC_END0_STAGE_STATUSOUT        0xf
#define MGC_END0_STAGE_STALL_BIT	0x10

/* obsolete */
#define MGC_END0_STAGE_DATAIN		MGC_END0_STAGE_TX
#define MGC_END0_STAGE_DATAOUT		MGC_END0_STAGE_RX


typedef enum
{
  MGC_STATE_DEFAULT,
  MGC_STATE_ADDRESS,
  MGC_STATE_CONFIGURED
} MGC_DeviceState;

/* failure codes */
#define MUSB_ERR_WAITING	1
#define MUSB_ERR_VBUS		-1
#define MUSB_ERR_BABBLE		-2
#define MUSB_ERR_CORRUPTED	-3
#define MUSB_ERR_IRQ		-4
#define MUSB_ERR_SHUTDOWN	-5
#define MUSB_ERR_RESTART	-6

/*************************** REGISTER ACCESS ********************************/

#define MGC_SelectEnd( _bEnd) \
    MGC_Write8(MGC_O_HDRC_INDEX, _bEnd)
#define MGC_ReadCsr8(_bOffset, _bEnd) \
    MGC_Read8((_bOffset + 0x10))
#define MGC_ReadCsr16(_bOffset, _bEnd) \
    MGC_Read16((_bOffset + 0x10))
#define MGC_WriteCsr8(_bOffset, _bEnd, _bData) \
    MGC_Write8((_bOffset + 0x10), _bData)
#define MGC_WriteCsr16(_bOffset, _bEnd, _bData) \
    MGC_Write16((_bOffset + 0x10), _bData)


/************************** ULPI Registers ********************************/

/* Added in HDRC 1.9(?) & MHDRC 1.4 */
/* ULPI pass-through */
#define MGC_O_HDRC_ULPI_VBUSCTL	0x70
#define MGC_O_HDRC_ULPI_REGDATA 0x74
#define MGC_O_HDRC_ULPI_REGADDR 0x75
#define MGC_O_HDRC_ULPI_REGCTL	0x76

/* extended config & PHY control */
#define MGC_O_HDRC_ENDCOUNT	0x78
#define MGC_O_HDRC_DMARAMCFG	0x79
#define MGC_O_HDRC_PHYWAIT	0x7A
#define MGC_O_HDRC_PHYVPLEN	0x7B	/* units of 546.1 us */
#define MGC_O_HDRC_HSEOF1	0x7C	/* units of 133.3 ns */
#define MGC_O_HDRC_FSEOF1	0x7D	/* units of 533.3 ns */
#define MGC_O_HDRC_LSEOF1	0x7E	/* units of 1.067 us */

/* Added in HDRC 1.9(?) & MHDRC 1.4 */
/* ULPI */
#define MGC_M_ULPI_VBUSCTL_USEEXTVBUSIND    0x02
#define MGC_M_ULPI_VBUSCTL_USEEXTVBUS	    0x01
#define MGC_M_ULPI_REGCTL_INT_ENABLE	    0x08
#define MGC_M_ULPI_REGCTL_READNOTWRITE	    0x04
#define MGC_M_ULPI_REGCTL_COMPLETE	    0x02
#define MGC_M_ULPI_REGCTL_REG		    0x01
/* extended config & PHY control */
#define MGC_M_ENDCOUNT_TXENDS	0x0f
#define MGC_S_ENDCOUNT_TXENDS	0
#define MGC_M_ENDCOUNT_RXENDS	0xf0
#define MGC_S_ENDCOUNT_RXENDS	4
#define MGC_M_DMARAMCFG_RAMBITS	0x0f	    /* RAMBITS-1 */
#define MGC_S_DMARAMCFG_RAMBITS	0
#define MGC_M_DMARAMCFG_DMACHS	0xf0
#define MGC_S_DMARAMCFG_DMACHS	4
#define MGC_M_PHYWAIT_WAITID	0x0f	    /* units of 4.369 ms */
#define MGC_S_PHYWAIT_WAITID	0
#define MGC_M_PHYWAIT_WAITCON	0xf0	    /* units of 533.3 ns */
#define MGC_S_PHYWAIT_WAITCON	4

/****************************** FUNCTIONS ********************************/

#define MUSB_HST_MODE(_pthis) { (_pthis)->bIsHost=TRUE; (_pthis)->bIsDevice=FALSE; \
	(_pthis)->bFailCode=0; }
#define MUSB_DEV_MODE(_pthis) { (_pthis)->bIsHost=FALSE; (_pthis)->bIsDevice=TRUE; \
	(_pthis)->bFailCode=0; }
#define MUSB_OTG_MODE(_pthis) { (_pthis)->bIsHost=FALSE; (_pthis)->bIsDevice=FALSE; \
	(_pthis)->bFailCode=MUSB_ERR_WAITING; }
#define MUSB_ERR_MODE(_pthis, _cause) { (_pthis)->bIsHost=FALSE; (_pthis)->bIsDevice=FALSE; \
	(_pthis)->bFailCode=_cause; }

#define MUSB_IS_ERR(_x) ( (_x)->bFailCode<0 )
#define MUSB_IS_HST(_x) ( !MUSB_IS_ERR(_x) && (_x)->bIsHost && !(_x)->bIsDevice )
#define MUSB_IS_DEV(_x) ( !MUSB_IS_ERR(_x) && !(_x)->bIsHost && (_x)->bIsDevice )
#define MUSB_IS_OTG(_x) ( !MUSB_IS_ERR(_x) && !(_x)->bIsHost && !(_x)->bIsDevice )

#define MUSB_MODE(_x) ( MUSB_IS_HST(_x)?"HOST":( MUSB_IS_DEV(_x)?"FUNCTION":(MUSB_IS_OTG(_x)?"OTG":"ERROR")) )

#define HDRC_IS_HST(_x) (  MGC_Read8((_x)->pRegs, MGC_O_HDRC_DEVCTL)&MGC_M_DEVCTL_HM )
#define HDRC_IS_DEV(_x) (  !HDRC_IS_HST(_x) )


/************************** Ep Configuration ********************************/


#define MUSB_EPD_AUTOCONFIG	0

#define MUSB_EPD_T_CNTRL	1
#define MUSB_EPD_T_ISOC		2
#define MUSB_EPD_T_BULK		3
#define MUSB_EPD_T_INTR		4

#define MUSB_EPD_D_INOUT	0
#define MUSB_EPD_D_TX		1
#define MUSB_EPD_D_RX		2

/******************************** TYPES *************************************/
/**
 * MGC_LinuxLocalEnd.
 * Local endpoint resource.
 * @field Lock spinlock
 * @field pUrb current URB
 * @field urb_list list
 * @field dwOffset current buffer offset
 * @field dwRequestSize how many bytes were last requested to move
 * @field wMaxPacketSizeTx local Tx FIFO size
 * @field wMaxPacketSizeRx local Rx FIFO size
 * @field wPacketSize programmed packet size
 * @field bIsSharedFifo TRUE if FIFO is shared between Tx and Rx
 * @field bAddress programmed bus address
 * @field bEnd programmed remote endpoint address
 * @field bTrafficType programmed traffic type
 * @field bIsClaimed TRUE if claimed
 * @field bIsTx TRUE if current direction is Tx
 * @field bIsReady TRUE if ready (available for new URB)
 */
typedef struct
{
    uint8_t bEnd; /* ep number */

	uint8_t bBusyCompleting; /* TRUE on Tx when the current urb is completing */

    unsigned int dwOffset; 		/* offset int the current request */
    unsigned int dwRequestSize; /* request size */
    unsigned int dwIsoPacket;
    unsigned int dwWaitFrame;
    uint8_t bRetries;	

    uint16_t wMaxPacketSizeTx;
    uint16_t wMaxPacketSizeRx;
    uint16_t wPacketSize;
    uint8_t bDisableDma; /* not used now! */
    uint8_t bIsSharedFifo;
	
	/* softstate, used from find_end() to determine a good match */
    uint8_t bRemoteAddress;
    uint8_t bRemoteEnd;	
    uint8_t bTrafficType;
    uint8_t bIsClaimed; /* only for isoc and int traffic */
    uint8_t bIsTx;
    uint8_t bIsReady;	
	uint8_t bStalled; /* the ep has been halted */	
} MGC_LinuxLocalEnd;

/**
 * MGC_LinuxCd.
 * Driver instance data.
 * @field Lock spinlock
 * @field Timer interval timer for various things
 * @field pBus pointer to Linux USBD bus
 * @field RootHub virtual root hub
 * @field PortServices services provided to virtual root hub
 * @field pRootDevice root device pointer, to track connection speed
 * @field nIrq IRQ number (needed by free_irq)
 * @field nIsPci TRUE if PCI
 * @field bIsMultipoint TRUE if multi-point core
 * @field bIsHost TRUE if host
 * @field bIsDevice TRUE if peripheral
 * @field nIackAddr IACK address (PCI only)
 * @field nIackSize size of IACK PCI region (needed by release_region)
 * @field nRegsAddr address of registers PCI region (needed by release_region)
 * @field nRegsSize size of registers region (needed by release_region)
 * @field pIack pointer to mapped IACK region (PCI only)
 * @field pRegs pointer to mapped registers
 */
typedef struct
{
    int nIrq;
	int nIrqType;
	
    int nBabbleCount;
    void* pRegs;

    MGC_LinuxLocalEnd aLocalEnd[MUSB_C_NUM_EPS];
	
    uint16_t wEndMask;
    uint8_t bEndCount;
    uint8_t bRootSpeed;
    uint8_t bIsMultipoint;
    uint8_t bIsHost;
    uint8_t bIsDevice;
    uint8_t bIgnoreDisconnect; /* during bus resets I got fake disconnects */
    uint8_t bVbusErrors; /* bus errors found */

    int bFailCode; /* one of MUSB_ERR_* failure code */

    uint8_t bBulkTxEnd;
    uint8_t bBulkRxEnd;
    uint8_t bBulkSplit;
    uint8_t bBulkCombine;

    uint8_t bEnd0Stage; /* end0 stage while in host or device mode */

    uint8_t bDeviceState;
    uint8_t bIsSelfPowered;
    uint8_t bSetAddress;
    uint8_t bAddress;
    uint8_t bTestMode;
    uint8_t bTestModeValue;

    /* Endpoint 0 buffer and its buffer code; can be customized for
     * devices that are not usign the default USB headers. Default
     * values are:
	 *
     * . pfFillBuffer is MGC_HdrcReadUSBControlRequest()
     * . pEnd0Buffer is an instance of MGC_End0Buffer
     */
    int (*pfReadHeader)(void*, uint16_t); /* NULL==MGC_HdrcReadUSBControlRequest*/
    uint8_t pEnd0Buffer[MUSB_MAX_END0_PACKET]; /* this is the buffer, default implementation uses MGC_End0Buffer */

    /* compatibility, need to be osoleted used from gstorage */
    uint16_t wEnd0Offset;	

} MGC_LinuxCd;

typedef struct {
	/* this memory is currently wasted with gstorage */
    unsigned int dwRequestSize;
    uint16_t wPacketSize;
    uint8_t bTrafficType;
    uint8_t bIsTx;
	
    uint8_t bEndNumber;
    MGC_LinuxCd	*pThis;
	
    /* compatibility, need to be osoleted, used from gstorage */	
    unsigned int dwOffset;
} MGC_GadgetLocalEnd;


#define   FlushFIFOEpx()     MGC_Write8(0x12, 8)

/***************************** Glue it together *****************************/

extern void MGC_HdrcStart(MGC_LinuxCd* pThis);
extern void MGC_HdrcLoadFifo(uint8_t bEnd, uint16_t wCount, const uint8_t* pSource);
extern void MGC_HdrcUnloadFifo(uint8_t bEnd, uint16_t wCount, uint8_t* pDest);
	
extern MGC_LinuxCd* MGC_HdrcInitController(void* pRegs);
extern 	void MGC_HdrcConfigureEps(MGC_LinuxCd* pThis);

extern const uint8_t MGC_aTestPacket[MGC_TEST_PACKET_SIZE];

extern void* MGC_MallocEp0Buffer(const MGC_LinuxCd* pThis);
extern MGC_LinuxCd* MGC_GetDriverByName(const char *name);
extern void MGC_GadgetReset(MGC_LinuxCd* pThis);
extern void MGC_HdrcServiceDeviceDefaultEnd(MGC_LinuxCd* pThis);
extern void MGC_HdrcServiceDeviceTxAvail(MGC_LinuxCd* pThis, uint8_t bEnd);
extern void MGC_HdrcServiceDeviceRxReady(MGC_LinuxCd* pThis, uint8_t bEnd);

extern void dma_inv_range(unsigned int start, unsigned int end);
extern void dma_clean_range(unsigned int start, unsigned int end);
extern void dma_flush_range(unsigned int start, unsigned int end);

#define usb_printf(...) do {}while(0)

#define	reg_usb_Power		*((volatile uint8_t*)(USB_BASE + MUSB_POWER))
#define	reg_usb_Index		*((volatile uint8_t*)(USB_BASE + MUSB_INDEX))
#define	reg_usb_EP0			*((volatile uint8_t*)(USB_BASE + MUSB_IDX_CSR0L))
#define	reg_usb_End0		*((volatile uint8_t*)(USB_BASE + 0x20))
#define	reg_usb_End1		*((volatile uint8_t*)(USB_BASE + 0x24))
#define	reg_usb_End2		*((volatile uint8_t*)(USB_BASE + 0x28))
#define	reg_usb_End3		*((volatile uint8_t*)(USB_BASE + 0x2C))
#define	reg_usb_IntrInE	*((volatile uint8_t*)(USB_BASE + MUSB_INTRTXE))
#define	reg_usb_IntrOutE	*((volatile uint8_t*)(USB_BASE + MUSB_INTRRXE))
#define	reg_usb_IntrUSBE	*((volatile uint8_t*)(USB_BASE + MUSB_INTRUSBE))
#define	reg_usb_Faddr		*((volatile uint8_t*)(USB_BASE + MUSB_FADDR))
#define	reg_usb_Testmode	*((volatile uint8_t*)(USB_BASE + MUSB_TESTMODE))
#define	reg_usb_IntrUSB	*((volatile uint8_t*)(USB_BASE + MUSB_INTRUSB))
#define	reg_usb_IntrIn		*((volatile uint8_t*)(USB_BASE + MUSB_INTRTX))

#endif	/* multiple inclusion protection */

