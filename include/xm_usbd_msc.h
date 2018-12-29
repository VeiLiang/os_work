//****************************************************************************
//
//	Copyright (C) 2004-2005 Zhuo YongHong
// Copyright (C) 2006 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_usbd_msc.h
//				USB MassStorage interface
//
//	Revision history
//
//		2005.01.15	ZhuoYongHong add definition about USB MassStorage interface 
//
//****************************************************************************
#ifndef _XM_USBD_MSC_H_
#define _XM_USBD_MSC_H_

// MassStorage endpoint 定义
#define	_USBD_MSC_EP_IN_						3	
#define	_USBD_MSC_EP_OUT_						4

#define	MSC_EVENT_BULK_IN		0x02
#define	MSC_EVENT_BULK_OUT	0x04


#define	CBW_SIGNATURE	0x43425355
#define	CSW_SIGNATURE	0x53425355

//======================================================
#define	BULK_ONLY_MASS_STORAGE_RESET	0xFF
#define	GET_MAX_LUN						0xFE

//======================================================
#define	BULK_CBW		0x00
#define	BULK_IN			0x01
#define	BULK_OUT		0x02
#define	BULK_CSW		0x04
#define BULK_NORMAL	0xFF


// MassStorage disk definition 
#define SPI_MSC_OFFSET    		0x00300000//0x00100000
#define SPI_MSC_LENGTH    		0x00100000//0x02400000
#define SPI_MSC_SIZE		  	0x00100000	// 实际容量, 1MB
#define SPI_MSC_SECSIZE	  	512//4096



int msc_class_request (void);
void msc_endpoints_reset(void);
void msc_bulk_out_transfer (void);
void msc_bulk_in_transfer (void);
void msc_event_handler (unsigned int event);

void usbd_msc_init (void);


// 将MSC磁盘完整内容写入到SD卡文件
int usbd_msc_save_disk_into_file (const char *disk_filename);

// 从SD卡文件恢复MSC磁盘内容
int usbd_msc_load_disk_from_file (const char *disk_filename);

// 1 锁定设备,   CDROM
// 0 非锁定设备, UDISK
void usbd_msc_lock_device (int lock);

#endif	// _USBD_MSC_H_