/*
 * (C) Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __COMMON_H_
#define __COMMON_H_	1

typedef unsigned char		u_char;
typedef unsigned short		u_short;
typedef unsigned int			u_int;
typedef unsigned long		u_long;

typedef unsigned char		uchar;
typedef unsigned short		ushort;
typedef unsigned int  		uint;
typedef volatile unsigned long	vu_long;
typedef unsigned long		ulong;

//typedef int			int32_t;
typedef volatile unsigned short vu_short;
typedef volatile unsigned char	vu_char;

typedef signed char __s8;
typedef unsigned char __u8;

typedef signed short __s16;
typedef unsigned short __u16;

typedef signed int __s32;
typedef unsigned int __u32;



typedef signed char s8;
//typedef unsigned char u8;

typedef signed short s16;
//typedef unsigned short u16;

typedef signed int s32;
//typedef unsigned int u32;

typedef signed long long s64;
typedef signed long long int64_t;
typedef signed long long off64_t;

typedef unsigned long long u64;

typedef		__u8		uint8_t;
typedef		__u16		uint16_t;
typedef 	__s8		int8_t;
//typedef		__u32		uint32_t;
typedef unsigned int dma_addr_t;

typedef unsigned long phys_addr_t;
typedef unsigned long phys_size_t;



#include <string.h>
#include <stdio.h>

//#define DEBUG

#undef DEBUG
#ifdef	DEBUG
#define debug(fmt,args...)	printf (fmt ,##args)
#define debugX(level,fmt,args...) if (DEBUG>=level) printf(fmt,##args);
#else
#define debug(fmt,args...)
#define debugX(level,fmt,args...)
#endif	/* DEBUG */

#define error(fmt, args...) do {					\
		printf("ERROR: " fmt "\nat %s:%d/%s()\n",		\
			##args, __FILE__, __LINE__, __func__);		\
} while (0)

#ifndef BUG
#define BUG() do { \
	printf("BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __FUNCTION__); \
	panic("BUG!"); \
} while (0)
#define BUG_ON(condition) do { if (unlikely((condition)!=0)) BUG(); } while(0)
#endif /* BUG */


#define min(x,y) ((x)<(y)?(x):(y))

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

/* timer.c*/
typedef enum {
	SDMMC_CLK=0,
	SPI_CLK,
	IIS_CLK,
	TIMER0_CLK,
	TIMER1_CLK,
	TIMER2_CLK,
	TIMER3_CLK,
	IRDA_CLK,
	APB_CLK,
	UART_CLK,

}clk_enum;



/* lib/time.c */
void	udelay        (unsigned long);



#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define ROUND(a,b)		(((a) + (b)) & ~((b) - 1))
#define DIV_ROUND(n,d)		(((n) + ((d)/2)) / (d))
#define DIV_ROUND_UP(n,d)	(((n) + (d) - 1) / (d))
#define roundup(x, y)		((((x) + ((y) - 1)) / (y)) * (y))

#define ALIGN(x,a)		__ALIGN_MASK((x),(typeof(x))(a)-1)
#define __ALIGN_MASK(x,mask)	(((x)+(mask))&~(mask))

unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base);


#define uswap_16(x) \
	((((x) & 0xff00) >> 8) | \
	 (((x) & 0x00ff) << 8))
#define uswap_32(x) \
	((((x) & 0xff000000) >> 24) | \
	 (((x) & 0x00ff0000) >>  8) | \
	 (((x) & 0x0000ff00) <<  8) | \
	 (((x) & 0x000000ff) << 24))
#define _uswap_64(x, sfx) \
	((((x) & 0xff00000000000000##sfx) >> 56) | \
	 (((x) & 0x00ff000000000000##sfx) >> 40) | \
	 (((x) & 0x0000ff0000000000##sfx) >> 24) | \
	 (((x) & 0x000000ff00000000##sfx) >>  8) | \
	 (((x) & 0x00000000ff000000##sfx) <<  8) | \
	 (((x) & 0x0000000000ff0000##sfx) << 24) | \
	 (((x) & 0x000000000000ff00##sfx) << 40) | \
	 (((x) & 0x00000000000000ff##sfx) << 56))

# define be16_to_cpu(x)		uswap_16(x)

#endif	/* __COMMON_H_ */
