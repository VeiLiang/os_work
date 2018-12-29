//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_rom.h
//	  constant，macro, data structure，function protocol definition of lowlevel rom interface 
//
//	Revision history
//
//		2010.09.08	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_ROM_H_
#define _XM_ROM_H_

#if defined (__cplusplus)
	extern "C"{
#endif

#define	MAX_FLAG_PNG_SIZE		0x10000	// 最大标志水印PNG文件字节大小


typedef struct _XM_ROMBIN {
	unsigned char	ID[8];			// "XMROMID"
	unsigned long	VERSION;			// 0x00010001
	unsigned char*	XML_BASE;
	unsigned long	XML_SIZE;
	unsigned char*	ROM_BASE;
	unsigned long	ROM_SIZE;
} XM_ROMBIN;


// ROM初始化
XMBOOL	XM_RomInit (void);

// ROM关闭
XMBOOL	XM_RomExit (void);

// 根据ROM偏移获取ROM的句柄(内存)地址
HANDLE XM_RomAddress (DWORD dwOffset);

// 根据ROM偏移获取ROM的字节大小
DWORD XM_RomLength (DWORD dwOffset);

// 根据XML偏移获取XML的句柄(内存)地址
HANDLE XM_XmlAddress (DWORD dwOffset);

// 从内部存储器(一般指SPI FLASH)读取标志PNG水印图片
// 0   读取成功
// -1  读取失败
int XM_FlagWaterMarkPngImageLoad (char *lpFlagBuffer, int cbFlagLength);

// 向内部存储器(一般指SPI FLASH)写入标志PNG水印图片
// 0   读取成功
// -1  读取失败
int XM_FlagWaterMarkPngImageSave (char *lpFlagBuffer, int cbFlagLength);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_ROM_H_