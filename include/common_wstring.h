//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: common_wstring.h
//	  ¿í×Ö·û´®²Ù×÷º¯Êý
//
//	Revision history
//
//		2010.09.09	ZhuoYongHong Initial version
//
//****************************************************************************

#ifndef _COMMON_WSTRING_H_
#define _COMMON_WSTRING_H_

#include <xm_type.h>

#ifdef __cplusplus
extern "C" { 
#endif

WCHAR*	wstrchr		(const WCHAR *wString, WCHAR wCode);
WCHAR*	wstrrchr		(const WCHAR *wString, WCHAR wCode);
WCHAR*	wstrstr		(const WCHAR *wString, const WCHAR *wstrSet);
SHORT		wstrcmp		(const WCHAR *wString1, const WCHAR *wString2);
SHORT		wstrncmp		(const WCHAR *wString1, const WCHAR *wString2, WORD nCount);
WCHAR*	wstrcpy		(WCHAR *wstrDest, const WCHAR *wstrSource);
WCHAR*	wstrncpy		(WCHAR *wstrDest, const WCHAR *wstrSource, WORD nCount);
WCHAR*	wstrcat		(WCHAR *wstrDest, const WCHAR *wstrSource);
WCHAR*	wstrncat		(WCHAR *wstrDest, const WCHAR *wstrSource, WORD nCount);
WCHAR*	wstrappend	(WCHAR *pDst, WCHAR wc);
WORD		wstrlen		(const WCHAR *wString);
WCHAR*	wstrrev		(WCHAR *wString);

WCHAR*	astr2wstr	(WCHAR *wstrDst, const CHAR *strSrc);
CHAR*		wstr2astr	(CHAR *strDst, const WCHAR *wstrSrc);

DWORD		wstr2dword	(const WCHAR *pwsSrc);
void		dword2wstr	(WCHAR *pwsDst, DWORD dwSrc, WORD uWidth);

int		UTF8toUTF16 (void *lpUtf8Buf, void *lpUnicodeBuf, int *lpcbUtf8Buf, int cbUnicodeBuf);


#ifdef __cplusplus
}
#endif

#endif
