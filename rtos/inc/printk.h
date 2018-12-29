#ifndef _PRINTK_H_
#define _PRINTK_H_

#include <stdio.h>
#include <stdlib.h>
#include "xm_printf.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _DEBUG
#define TRACE                   printk
#define TRACE0(sz)              printk(sz)
#define TRACE1(sz, p1)          printk(sz, p1)
#define TRACE2(sz, p1, p2)      printk(sz, p1, p2)
#define TRACE3(sz, p1, p2, p3)  printk(sz, p1, p2, p3)
#else
#define TRACE                   1?(void)0:printk
#define TRACE0(sz)
#define TRACE1(sz, p1)
#define TRACE2(sz, p1, p2)
#define TRACE3(sz, p1, p2, p3)
#endif

int printk(const char *fmt,...);

#define	printk	XM_printf

#ifdef __cplusplus
}
#endif

#endif

