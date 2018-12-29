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
// 系统(资源)语言代码页定义

#define	FML_LANG_CHINESE_SIMPLIFIED	0x0001	// 中文(简体)
#define	FML_LANG_CHINESE_TRADITIONAL	0x0002	// 中文(繁体)
#define	FML_LANG_CHINESE_HONGKONG		0x0003	// 中文(香港)
#define	FML_LANG_ENGLISH					0x0004	// 英语
#define	FML_LANG_FRENCH					0x0005	// 法语
#define	FML_LANG_RUSSIAN					0x0006	// 俄语
#define	FML_LANG_SPANISH					0x0007	// 西班牙语
#define	FML_LANG_JAPANESE					0x0008	// 日语
#define	FML_LANG_KOREAN					0x0009	// 韩语
#define	FML_LANG_ITALIAN					0x000A	// 意大利语
#define	FML_LANG_GERMAN					0x000B	// 德语
#define	FML_LANG_ARABIC					0x000C	//	阿拉伯语	
#define	FML_LANG_MALAY						0x000D	// 马来语
#define	FML_LANG_INDONESIAN				0x000E	// 印度尼西亚语
#define	FML_LANG_THAI						0x000F	// 泰国语
#define	FML_LANG_UIGUR						0x0010	// 维吾尔语
#define	FML_LANG_TIBET						0x0011	// 藏语

// 20081028 ZhuoYongHong
// 增加UTF16/UTF8的语言ID
#define	FML_LANG_UTF16						0x0100	// UTF16 (little endian)
#define	FML_LANG_UTF16_BIG_ENDIAN		0x0101	// UTF16 (big endian)
#define	FML_LANG_UTF8						0x0102	// UTF8  

// 获取系统语言代码页
WORD	XM_GetSystemCodePage		(void);
// 获取资源语言代码页
WORD	XM_GetResourceCodePage	(void);
// 设置资源语言代码页，返回值为原来的代码页
WORD	XM_SetResourceCodePage	(WORD wCodePage);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */


#endif	// _XM_LANG_H_
