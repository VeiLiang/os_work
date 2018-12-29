//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xmtype.h
//	  constant��macro & basic typedef definition of X-Mini Window System
//
//	Revision history
//
//		2010.08.31	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_TYPE_H_
#define _XM_TYPE_H_


#if defined(_WINDOWS) && defined(WIN32)

// ��������VC���������漰������

#pragma warning(disable:4068) // ����ʾ4068�ž�����Ϣ (warning C4068: unknown pragma)
#pragma warning(disable:4100) // ����ʾ4100�ž�����Ϣ (unreferenced formal parameter)
#pragma warning(error:4090)   // ������ C4090: '=' : different 'const' qualifiers
//#pragma warning(error:4701)	// ������ C4701: local variable 'xxx' may be used without having been initialized
#pragma warning(error:4706)	// ������ C4706: assignment within conditional expression
#pragma warning(error:4716)	// ������ C4716: must return a value
#pragma warning(error:4013)	// ������ C4013: undefined; assuming extern returning int
#pragma warning(error:4028)	// formal parameter nnn different from declaration
#pragma warning(error:4245)	// 'function' : conversion from 'const int ' to 'unsigned short ', signed/unsigned mismatch
#pragma warning(error:4020)	// ������ C4020: 'xxx' : too many actual parameters
#pragma warning(error:4244)	// ������ C4244: warning C4244: '=' : conversion from 'short ' to 'unsigned char ', possible loss of data

#endif	// #if defined(_WINDOWS) && defined(WIN32)

#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif

typedef signed char			i8_t;
typedef signed short			i16_t;
typedef signed int			i32_t;
typedef unsigned char		u8_t;
typedef unsigned short		u16_t;
typedef unsigned int		u32_t;

#ifndef VOID
#define VOID 					void
typedef signed char 			CHAR;
typedef signed short 		SHORT;
typedef long 					LONG;
#endif

typedef unsigned char 		UCHAR;
typedef unsigned short 		USHORT;
typedef unsigned long		ULONG;
typedef unsigned int			UINT;


typedef unsigned long      DWORD;
typedef unsigned char      BYTE;
typedef unsigned short     WORD;
typedef float              FLOAT;

#ifndef _WINDEF_
typedef unsigned short 		WCHAR;
#endif

#ifdef WIN32
typedef __int64				XMINT64;
#else
typedef signed long long	XMINT64;
#endif

#ifdef WIN32
typedef __int64				i64_t;
#else
typedef signed long long	i64_t;
#endif


#ifndef _XMCOORD_DEFINED_
#define _XMCOORD_DEFINED_
typedef signed int			XMCOORD;		// λ������
#endif

#ifndef _XMCOLOR_DEFINED_
#define _XMCOLOR_DEFINED_
typedef unsigned long		XMCOLOR;		// ��ɫ����
#endif

#ifndef _XMBOOL_DEFINED_
#define _XMBOOL_DEFINED_
typedef unsigned char		XMBOOL;		// BOOL����
#endif

typedef VOID *					HANDLE;

// ���Ͷ���
#undef NULL
#define NULL    				(0)

/*
#ifndef _WCHAR_T_DEFINED
#undef wchar_t
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif*/

// �궨��
#define MAKEWORD(a, b)     ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#define MAKELONG(a, b)     ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
#define LOWORD(l)          ((WORD)(l))
#define HIWORD(l)          ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#define LOBYTE(w)          ((BYTE)(w))
#define HIBYTE(w)          ((BYTE)(((WORD)(w) >> 8) & 0xFF))

#define XMPALETTEINDEX(i)   ((XMCOLOR)(0x01000000 | (DWORD)(WORD)(i)))

#define XM_MAX_PATH			127			/* maximum path */


// ���ݽṹ����
typedef struct tgXMPOINT {
	XMCOORD x;	// x����
	XMCOORD y;	// y����
} XMPOINT;

// ���νṹ
typedef struct tagXMRECT {
	XMCOORD left;		// �������Ͻ�X����
	XMCOORD top;		// �������Ͻ�Y����
	XMCOORD right;		// �������½�X����
	XMCOORD bottom;	// �������½�Y����
} XMRECT;

// �ߴ�ṹ
typedef struct tagXMSIZE {
    XMCOORD cx;
    XMCOORD cy;
} XMSIZE;


#endif	// _XM_TYPE_H_
