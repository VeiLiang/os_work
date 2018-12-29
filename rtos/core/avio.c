#include "xm_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>
#include <assert.h>

#include "xm_h264_file.h"


#include "FS.h"

typedef	unsigned int uint_32;
typedef	unsigned int uint32_t;



int isatty(void)
{
	//printf ("isatty\n");
	return 1;
}

void *stderr;

void nanosleep (void)
{
	printf ("nanosleep\n");
}

static int _errno = -1;
int* __errno_location (void)
{
	return &_errno;
}

int gettimeofday(void)
{
	printf ("gettimeofday\n");
	return 0;
}

#include <stdio.h>
#include <stdarg.h>

int __isoc99_sscanf (const char *str, const char *fmt,...)
{
	va_list argptr;
	int cnt;
	
	// printf ("__isoc99_sscanf str=%s, fmt=%s\n", str, fmt);
	
	va_start(argptr, fmt);
	cnt = vsscanf(str, fmt, argptr);
	va_end(argptr);
	
	return cnt;
}

void fputs (const char *s, void *fp)
{
	printf ("%s\n", s);
}

void fprintf (void)
{
	printf ("fprintf\n");
}

int fwrite (void *d, int size, int nlem, void *fd)
{
	printf ("fwrite\n");
	return 0;
}

void fputc (void)
{
	printf ("fputc\n");
}


/*
int posix_memalign(void **memptr, size_t alignment, size_t size)
{
	unsigned int addr = (unsigned int)malloc (size + alignment - 1);
	printf ("posix_memalign alignment=%d, size=%d, addr=%08x\n", alignment, size, addr);
	if(addr == 0)
	{
		printf ("posix_memalign alloc failed, size=%d\n", size);
		return -1;
	}
	else
	{
		addr += alignment - 1;
		addr &= ~(alignment - 1);
		*(unsigned int *)memptr = addr;
		
		//printf ("posix_memalign f alignment=%d, size=%d, p_addr=%08x\n", alignment, size, addr);
		
		return 0;
	}
}*/

union av_intfloat32 {
    uint32_t i;
    float    f;
};
static  uint32_t av_float2int(float f)
{
    union av_intfloat32 v;
    v.f = f;
    return v.i;
}

int __isinf (float x)

{
    uint32_t v = av_float2int(x);
    if ((v & 0x7f800000) != 0x7f800000)
        return 0;
    return !(v & 0x007fffff);
}

int __isnan(float x)
{
    uint32_t v = av_float2int(x);
    if ((v & 0x7f800000) != 0x7f800000)
        return 0;
    return v & 0x007fffff;
}


#include <ctype.h>
/*
int av_isspace( int c )
{
	return isspace (c);
}


int av_isdigit( int c )
{
	return isdigit (c);
}*/


unsigned int iar_random(void)
{
	return rand();
}

 int __xpg_strerror_r(int errnum, char *buf, size_t n)
 {
	return -1; 
 }

int __sync_add_and_fetch_4 (volatile int *ptr, int inc)
{
    *ptr += inc;
    return *ptr;
}


// These builtins perform an atomic compare and swap. 
// That is, if the current value of *ptr is oldval, then write newval into *ptr.
// The ¡°val¡± version returns the contents of *ptr before the operation. 
int __sync_val_compare_and_swap_4 (int *ptr, int  oldval, int  newval)
{
	int v = *ptr;
	if(*ptr == oldval)
	{
		*ptr = newval;
	}
	return v;
}


// Allocate aligned memory
// 	The posix_memalign() function allocates size bytes aligned on a boundary specified by alignment. 
// 	It returns a pointer to the allocated memory in memptr.

// 	The buffer allocated by posix_memalign() is contiguous in virtual address space, 
//		but not physical memory. Since some platforms don't allocate memory in 4 KB page sizes, 
//		you shouldn't assume that the memory allocated will be physically contiguous 
//		if you specify a size of 4 KB or less.
//
//	memptr
//		A pointer to a location where posix_memalign() can store a pointer to the memory.
//	alignment
//		The alignment to use for the memory. 
//		This must be a multiple of size( void * ) that's also a power of 2.
//	size
//		The size, in bytes, of the block to allocate.
//
// Returns:
//	0
//		Success.
//	-1
//		An error occurred (errno is set).
//	Errors:
//		EINVAL
//			The value of alignment isn't a multiple of size( void * ) that's also a power of 2.
//		ENOMEM
//			There's insufficient memory available with the requested alignment.
int posix_memalign (void **memptr,
                    size_t alignment,
                    size_t size)
{
	return -1;
}
