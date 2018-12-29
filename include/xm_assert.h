//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xmassert.h
//	  exception process
//
//	Revision history
//
//		2010.09.02	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_ASSERT_H_
#define _XM_ASSERT_H_

#if defined (__cplusplus)
	extern "C"{
#endif

#include <xm_type.h>


#ifdef NDEBUG

#define	XM_ASSERT(exp)		((void)0)

#else

#if defined(PCVER)

extern	void	_assert (void *, void *, unsigned int);
#define	XM_ASSERT(exp)		(void)( (exp) || (_assert(#exp, __FILE__, __LINE__), 0) )

#else

extern	void	_xm_assert_ (void *, void *, unsigned int);

//#define	XM_ASSERT(exp)  ( (exp) || (_xm_assert_(#exp, __FILE__, __LINE__), 0) )
#define	XM_ASSERT(exp)

#endif

#endif

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_USER_H_
