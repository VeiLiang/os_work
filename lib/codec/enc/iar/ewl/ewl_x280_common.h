/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2006 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Abstract : Hantro 6280/7280/8270/8290 Encoder Wrapper Layer for OS services
--
------------------------------------------------------------------------------*/
#ifndef __EWL_X280_COMMON_H__
#define __EWL_X280_COMMON_H__

#include <stdio.h>
#include "rtos.h"

//#define	TRACE_EWL

/* Macro for debug printing */
#undef PTRACE
#ifdef TRACE_EWL
#   include <stdio.h>
//#   define PTRACE(...) {printf("%s:%d:",__FILE__,__LINE__);printf(__VA_ARGS__);}
#   define PTRACE(...) {printf("%d:",__LINE__);printf(__VA_ARGS__);}
#else
#   define PTRACE(...)  /* no trace */
#endif

#define ASIC_STATUS_SLICE_READY         0x100
#define ASIC_STATUS_TEST_COPY_RDY       0x80
#define ASIC_STATUS_TEST_IRQ            0x40
#define ASIC_STATUS_BUFF_FULL           0x20
#define ASIC_STATUS_RESET               0x10
#define ASIC_STATUS_ERROR               0x08
#define ASIC_STATUS_FRAME_READY         0x04
#define ASIC_IRQ_LINE                   0x01

#define ASIC_STATUS_ALL     (ASIC_STATUS_SLICE_READY |\
                             ASIC_STATUS_TEST_IRQ |\
                             ASIC_STATUS_TEST_COPY_RDY |\
                             ASIC_STATUS_FRAME_READY | \
                             ASIC_STATUS_BUFF_FULL | \
                             ASIC_STATUS_RESET | \
                             ASIC_STATUS_ERROR)

#ifndef SDRAM_LM_BASE
#define SDRAM_LM_BASE               0x80000000
#endif

/* EWL internal information for Linux */

typedef struct

{
    
	u32 clientType;
    
//	int fd_mem;              /* /dev/mem */
    
//	int fd_enc;              /* /dev/hx280 */
    
//	int fd_memalloc;         /* /dev/memalloc */
    
	u32 regSize;             /* IO mem size */
    
	u32 regBase;
    
	volatile u32 *pRegBase;  /* IO mem base */
    
	
//	int sigio_needed;

} hx280ewl_t;

#endif /* __EWLX280_COMMON_H__ */
