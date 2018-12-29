//****************************************************************************
//
//	Copyright (C) 2010~2017 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_flash_space_define.h
//	  SPI Flash space usage definition
//
//	Revision history
//
//		2010.09.01	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_FLASH_SPACE_DEFINE_H_
#define _XM_FLASH_SPACE_DEFINE_H_


#if defined (__cplusplus)
	extern "C"{
#endif

// Flash�ռ����
// 0x000000 ~ 0x000FFF		// 4KB  Loader
// 0x001000 ~ 0x1FEFFF		// RTOS & APP & ROM
// 0x1FF000 ~ 0x1FFFFF		// 4KB  Meun Data

// Loader ��������ַ����С
#define	XM_FLASH_LOADER_BASE			0x000000
#define	XM_FLASH_LOADER_SIZE			0x001000		// 4KB
		
#if FLASH_3MB
// Menu Data �˵������ݿ��ַ����С
#define	XM_FLASH_MENUDATA_BASE		(0x400000 - 0x001000)
#define	XM_FLASH_MENUDATA_SIZE		0x001000		// 4KB
		
// RTOS & ROM & APP ��ַ����С
#define	XM_FLASH_RTOS_BASE			0x001000
#define	XM_FLASH_RTOS_SIZE			(0x400000 - 0x001000 - 0x001000)		// (3MB - 8KB)
		
#else
// Menu Data �˵������ݿ��ַ����С
#define	XM_FLASH_MENUDATA_BASE		(0x200000 - 0x001000)
#define	XM_FLASH_MENUDATA_SIZE		0x001000		// 4KB
		
// RTOS & ROM & APP ��ַ����С
#define	XM_FLASH_RTOS_BASE			0x001000
#define	XM_FLASH_RTOS_SIZE			(0x200000 - 0x001000 - 0x001000)		// (2MB - 8KB)
#endif

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */
		
#endif	// _XM_FLASH_SPACE_DEFINE_H_

