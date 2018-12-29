//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_rom.h
//	  constant��macro, data structure��function protocol definition of lowlevel rom interface 
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

#define	MAX_FLAG_PNG_SIZE		0x10000	// ����־ˮӡPNG�ļ��ֽڴ�С


typedef struct _XM_ROMBIN {
	unsigned char	ID[8];			// "XMROMID"
	unsigned long	VERSION;			// 0x00010001
	unsigned char*	XML_BASE;
	unsigned long	XML_SIZE;
	unsigned char*	ROM_BASE;
	unsigned long	ROM_SIZE;
} XM_ROMBIN;


// ROM��ʼ��
XMBOOL	XM_RomInit (void);

// ROM�ر�
XMBOOL	XM_RomExit (void);

// ����ROMƫ�ƻ�ȡROM�ľ��(�ڴ�)��ַ
HANDLE XM_RomAddress (DWORD dwOffset);

// ����ROMƫ�ƻ�ȡROM���ֽڴ�С
DWORD XM_RomLength (DWORD dwOffset);

// ����XMLƫ�ƻ�ȡXML�ľ��(�ڴ�)��ַ
HANDLE XM_XmlAddress (DWORD dwOffset);

// ���ڲ��洢��(һ��ָSPI FLASH)��ȡ��־PNGˮӡͼƬ
// 0   ��ȡ�ɹ�
// -1  ��ȡʧ��
int XM_FlagWaterMarkPngImageLoad (char *lpFlagBuffer, int cbFlagLength);

// ���ڲ��洢��(һ��ָSPI FLASH)д���־PNGˮӡͼƬ
// 0   ��ȡ�ɹ�
// -1  ��ȡʧ��
int XM_FlagWaterMarkPngImageSave (char *lpFlagBuffer, int cbFlagLength);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_ROM_H_