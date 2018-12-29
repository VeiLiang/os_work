//****************************************************************************
//
//	Copyright (C) 2004-2005 Shenzhen Exceedspace Digital Technology Co.,LTD
//
//	Author	ZhuoYongHong
//
//	File name: xmlang.h
//	  language identifier definition
//
//	Revision history
//
//		2005.09.30	ZhuoYongHong initial version
//		2006.01.03  ZhuoYongHong re-define language identifier that compatible with Windows
//
//****************************************************************************

#ifndef	_XM_LANG_H_
#define	_XM_LANG_H_

#if defined (__cplusplus)
	extern "C"{
#endif

// constant definition
// ϵͳ(��Դ)���Դ���ҳ����

#define	FML_LANG_CHINESE_SIMPLIFIED	0x0001	// ����(����)
#define	FML_LANG_CHINESE_TRADITIONAL	0x0002	// ����(����)
#define	FML_LANG_CHINESE_HONGKONG		0x0003	// ����(���)
#define	FML_LANG_ENGLISH					0x0004	// Ӣ��
#define	FML_LANG_FRENCH					0x0005	// ����
#define	FML_LANG_RUSSIAN					0x0006	// ����
#define	FML_LANG_SPANISH					0x0007	// ��������
#define	FML_LANG_JAPANESE					0x0008	// ����
#define	FML_LANG_KOREAN					0x0009	// ����
#define	FML_LANG_ITALIAN					0x000A	// �������
#define	FML_LANG_GERMAN					0x000B	// ����
#define	FML_LANG_ARABIC					0x000C	//	��������	
#define	FML_LANG_MALAY						0x000D	// ������
#define	FML_LANG_INDONESIAN				0x000E	// ӡ����������
#define	FML_LANG_THAI						0x000F	// ̩����
#define	FML_LANG_UIGUR						0x0010	// ά�����
#define	FML_LANG_TIBET						0x0011	// ����

// 20081028 ZhuoYongHong
// ����UTF16/UTF8������ID
#define	FML_LANG_UTF16						0x0100	// UTF16 (little endian)
#define	FML_LANG_UTF16_BIG_ENDIAN		0x0101	// UTF16 (big endian)
#define	FML_LANG_UTF8						0x0102	// UTF8  

// ��ȡϵͳ���Դ���ҳ
WORD	XM_GetSystemCodePage		(void);
// ��ȡ��Դ���Դ���ҳ
WORD	XM_GetResourceCodePage	(void);
// ������Դ���Դ���ҳ������ֵΪԭ���Ĵ���ҳ
WORD	XM_SetResourceCodePage	(WORD wCodePage);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */


#endif	// _XM_LANG_H_
