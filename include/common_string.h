//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: common_string.h
//	  ×Ö·û´®²Ù×÷º¯Êý
//
//	Revision history
//
//		2010.09.09	ZhuoYongHong Initial version
//
//****************************************************************************

#ifndef _COMMON_STRING_H_
#define _COMMON_STRING_H_

#include <xm_type.h>

#ifdef __cplusplus
extern "C" { 
#endif

int xm_strlen (const char *s);
void * xm_memcpy	(void *_s1, const void *_s2, int _n);
void * xm_memset	(void *_s, int _c, int _n);
int xm_strcmpi(const char * dst, const char * src);
int xm_stricmp (const char * dst, const char * src);
int xm_strnicmp (const char * dst, const char * src, int count);


#ifdef __cplusplus
}
#endif

#endif
