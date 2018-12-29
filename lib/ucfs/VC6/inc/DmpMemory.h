#ifndef _DMPMEMORY_H
#define _DMPMEMORY_H

#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif


void *Dmpmalloc(unsigned nbytes);
#define	Dmpmalloc	OS_malloc	

void Dmpfree(void *ap);
#define	Dmpfree		OS_free

void *Dmpcalloc (unsigned int bytes,unsigned int size);
#define	Dmpcalloc	OS_calloc

void *Dmprealloc (void *ptr,unsigned int size);
#define	Dmprealloc	OS_realloc

#ifdef __cplusplus
}
#endif

#endif

