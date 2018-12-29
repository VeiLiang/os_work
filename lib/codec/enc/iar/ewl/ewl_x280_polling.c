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
--  Abstract : Encoder Wrapper Layer for 6280/7280/8270/8290 without interrupts
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"
#include "ewl.h"
//#include "ewl_linux_lock.h"
#include "ewl_x280_common.h"
#include "arkn141_codec.h"

//#include "hx280enc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <errno.h>

//#include <sys/types.h>
//#include <unistd.h>
//#include <pthread.h>

#ifndef EWL_NO_HW_TIMEOUT
#define EWL_WAIT_HW_TIMEOUT 2   /* HW IRQ timeout in seconds */
#endif

extern volatile u32 asic_status;



/*******************************************************************************
 Function name   : EWLWaitHwRdy
 Description     : Poll the encoder interrupt register to notice IRQ
 Return type     : i32 
 Argument        : void
*******************************************************************************/
i32 EWLWaitHwRdy(const void *inst, u32 *slicesReady)
{
#ifdef _ARKN141_H264_ENCODE_INTERRUPT_
	hx280ewl_t *enc = (hx280ewl_t *) inst;
	u32 prevSlicesReady = 0;
	volatile u32 irq_stats;
	
	assert(enc != NULL);
	
	switch(arkn141_codec_waiting_for_h264_encode_finished())
	{
		case 0:	// encode finished 
			/* Get the number of completed slices from ASIC registers. */
			if (slicesReady)
				*slicesReady = (enc->pRegBase[21] >> 16) & 0xFF;
			irq_stats = enc->pRegBase[1];
			PTRACE("EWLWaitHw: IRQ stat = %08x\n", irq_stats);
			asic_status = irq_stats; /* update the buffered asic status */
			//printf ("EWLWaitHw: OK!\n");
			return EWL_OK;
			
		case (-2):	// timeout
			printf ("EWLWaitHw: timeout!\n");
			return EWL_HW_WAIT_TIMEOUT;
			
		case (-1):
		default:
			printf ("EWLWaitHw: ERROR!\n");
			return EWL_HW_WAIT_ERROR;
	}
	
#else	
    hx280ewl_t *enc = (hx280ewl_t *) inst;
    volatile u32 irq_stats;
    u32 prevSlicesReady = 0;
    i32 ret = EWL_HW_WAIT_TIMEOUT;
    // struct timespec t;
    u32 timeout = 1000;     /* Polling interval in microseconds */
    int loop = 1000;         /* How many times to poll before timeout */

    assert(enc != NULL);

    PTRACE("EWLWaitHwRdy\n");
    (void) enc;

    /* The function should return when a slice is ready */
    if (slicesReady)
        prevSlicesReady = *slicesReady;

    if(timeout == (u32) (-1) )
    {
        loop = -1;   /* wait forever (almost) */
        timeout = 1000; /* 1ms polling interval */
    }

    //t.tv_sec = 0;
    //t.tv_nsec = timeout - t.tv_sec * 1000;
    //t.tv_nsec = 100 * 1000 * 1000;

    do
    {
        /* Get the number of completed slices from ASIC registers. */
        if (slicesReady)
            *slicesReady = (enc->pRegBase[21] >> 16) & 0xFF;

        irq_stats = enc->pRegBase[1];

        PTRACE("EWLWaitHw: IRQ stat = %08x\n", irq_stats);

        if((irq_stats & ASIC_STATUS_ALL))
        {
            /* clear IRQ and slice ready status */
            irq_stats &= (~(ASIC_STATUS_SLICE_READY|ASIC_IRQ_LINE));
            EWLWriteReg(inst, 0x04, irq_stats);

            ret = EWL_OK;
            loop = 0;
        }

        if (slicesReady)
        {
            if (*slicesReady > prevSlicesReady)
            {
                ret = EWL_OK;
                loop = 0;
            }
        }

        if (loop)
        {
            //if(nanosleep(&t, NULL) != 0)
            //    PTRACE("EWLWaitHw: Sleep interrupted!\n");
			  OS_Delay (1);
        }
    }
    while(loop--);

    asic_status = irq_stats; /* update the buffered asic status */

    if (slicesReady)
        PTRACE("EWLWaitHw: slicesReady = %d\n", *slicesReady);
    PTRACE("EWLWaitHw: asic_status = %x\n", asic_status);
    PTRACE("EWLWaitHw: OK!\n");
    return ret;
#endif	// _ARKN141_H264_ENCODE_INTERRUPT_
}
