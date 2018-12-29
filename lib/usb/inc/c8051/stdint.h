/******************************************************************
 *                                                                *
 *         Copyright Mentor Graphics Corporation 2004             *
 *                                                                *
 *                All Rights Reserved.                            *
 *                                                                *
 *    THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION *
 *  WHICH IS THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS   *
 *  LICENSORS AND IS SUBJECT TO LICENSE TERMS.                    *
 *                                                                *
 ******************************************************************/

/*
 * MUSBStack-S Keil/8051 C'99 definitions.
 * $Revision: 1.1 $
 */

#ifndef __C8051_STDINT_H__
#define __C8051_STDINT_H__

/* 7.18.1.1 */

/* exact-width signed integer types */
typedef signed char			int8_t;
typedef signed short			int16_t;
typedef signed long			int32_t;

/* exact-width unsigned integer types */
typedef unsigned char		uint8_t;
typedef unsigned short		uint16_t;
typedef unsigned long		uint32_t;

/* 7.18.1.2 */

/* smallest type of at least n bits */
/* minimum-width signed integer types */
typedef   int8_t		int_least8_t;
typedef   int16_t		int_least16_t;
typedef   int32_t		int_least32_t;

/* minimum-width unsigned integer types */
typedef uint8_t			uint_least8_t;
typedef uint16_t		uint_least16_t;
typedef uint32_t		uint_least32_t;

/* 7.18.1.3 */

/* fastest minimum-width signed integer types */
typedef   int8_t		int_fast8_t;
typedef   int16_t		int_fast16_t;
typedef   int32_t		int_fast32_t;

    /* fastest minimum-width unsigned integer types */
typedef uint8_t			uint_fast8_t;
typedef uint16_t		uint_fast16_t;
typedef uint32_t		uint_fast32_t;

/* 7.18.1.4 integer types capable of holding object pointers */
typedef long			intptr_t;
typedef	unsigned long		uintptr_t;

/* 7.18.1.5 greatest-width integer types */
typedef int32_t			intmax_t;
typedef uint32_t		uintmax_t;

/* 7.18.2.1 */

/* minimum values of exact-width signed integer types */

#define INT8_MIN                   -128
#define INT16_MIN                -32768
#define INT32_MIN          (~0x7fffffff)   /* -2147483648 is unsigned */

/* maximum values of exact-width signed integer types */
#define INT8_MAX                    127
#define INT16_MAX                 32767
#define INT32_MAX            2147483647

/* maximum values of exact-width unsigned integer types */
#define UINT8_MAX                   255
#define UINT16_MAX                65535
#define UINT32_MAX           4294967295u

/* 7.18.2.2 */

/* minimum values of minimum-width signed integer types */
#define INT_LEAST8_MIN                   -128
#define INT_LEAST16_MIN                -32768
#define INT_LEAST32_MIN          (~0x7fffffff)

/* maximum values of minimum-width signed integer types */
#define INT_LEAST8_MAX                    127
#define INT_LEAST16_MAX                 32767
#define INT_LEAST32_MAX            2147483647

/* maximum values of minimum-width unsigned integer types */
#define UINT_LEAST8_MAX                   255
#define UINT_LEAST16_MAX                65535
#define UINT_LEAST32_MAX           4294967295u

/* 7.18.2.3 */

/* minimum values of fastest minimum-width signed integer types */
#define INT_FAST8_MIN           (~0x7fffffff)
#define INT_FAST16_MIN          (~0x7fffffff)
#define INT_FAST32_MIN          (~0x7fffffff)

/* maximum values of fastest minimum-width signed integer types */
#define INT_FAST8_MAX             2147483647
#define INT_FAST16_MAX            2147483647
#define INT_FAST32_MAX            2147483647

/* maximum values of fastest minimum-width unsigned integer types */
#define UINT_FAST8_MAX            4294967295u
#define UINT_FAST16_MAX           4294967295u
#define UINT_FAST32_MAX           4294967295u

/* 7.18.2.4 */

/* minimum value of pointer-holding signed integer type */
#define INTPTR_MIN (~0x7fffffff)

/* maximum value of pointer-holding signed integer type */
#define INTPTR_MAX   2147483647

/* maximum value of pointer-holding unsigned integer type */
#define UINTPTR_MAX  4294967295u

/* 7.18.2.5 */

/* minimum value of greatest-width signed integer type */
#define INTMAX_MIN  (~0x7fffffffffffffffll)

/* maximum value of greatest-width signed integer type */
#define INTMAX_MAX   9223372036854775807ll

/* maximum value of greatest-width unsigned integer type */
#define UINTMAX_MAX 18446744073709551615ull

#endif	/* multiple inclusion protection */
