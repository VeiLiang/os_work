//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xmprintf.h
//	  ���Խӿ� 
//
//	Revision history
//
//		2010.09.08	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_PRINTF_H_
#define _XM_PRINTF_H_

#if defined (__cplusplus)
	extern "C"{
#endif

// ���Դ�ӡ�ӿ�
#if defined(HWVER)
#define	XM_printf		DrvSIO_printf
//#define	XM_printf		DrvSIO_SemiPrintf

#endif

void XM_printf (char *fmt, ...);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_PRINTF_H_