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
--  Description : System Wrapper Layer for Linux. Polling version.
--
------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: dwl_x170_linux_no_drv.c,v $
--  $Revision: 1.13 $
--  $Date: 2010/03/24 06:21:33 $
--
------------------------------------------------------------------------------*/

#include "basetype.h"
#include "dwl.h"
#include "dwl_iar.h"
#include "arkn141_codec.h"

//#include "memalloc.h"
//#include "dwl_linux_lock.h"

//#include "hx170dec.h"

//#include <sys/types.h>
//#include <sys/stat.h>
//#include <sys/mman.h>
//#include <sys/ioctl.h>
//#include <sys/timeb.h>

//#include <signal.h>
//#include <fcntl.h>
//#include <unistd.h>

//#include <pthread.h>
//#include <errno.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rtos.h"
#include "fs.h"
#include "xm_core.h"

#ifdef USE_EFENCE
#include "efence.h"
#endif


#define X170_SEM_KEY 0x8070

/* Decoder interrupt register */
#define X170_INTERRUPT_REGISTER_DEC     (1*4)
#define X170_INTERRUPT_REGISTER_PP      (60*4)

/* Logic module base address */
#define HXDEC_LOGIC_MODULE0_BASE   0x70070000

#define INT_EXPINT1                     10
#define INT_EXPINT2                     11
#define INT_EXPINT3                     12

/* these could be module params in the future */

#define DEC_IO_BASE                 HXDEC_LOGIC_MODULE0_BASE
#define DEC_IO_SIZE                 ((100+1) * 4)   /* bytes */

/* a mutex protecting the wrapper init */
//pthread_mutex_t x170_init_mutex = PTHREAD_MUTEX_INITIALIZER;

/* the decoder device driver nod */
//const char *dec_dev = DEC_MODULE_PATH;

/* the memalloc device driver nod */
//const char *mem_dev = MEMALLOC_MODULE_PATH;

/*------------------------------------------------------------------------------
    Function name   : DWLInit
    Description     : Initialize a DWL instance

    Return type     : const void * - pointer to a DWL instance

    Argument        : DWLInitParam_t * param - initialization params
------------------------------------------------------------------------------*/
const void *DWLInit(DWLInitParam_t * param)
{
    hX170dwl_t *dec_dwl;
    unsigned long base;

    assert(param != NULL);

    dec_dwl = (hX170dwl_t *) kernel_malloc(sizeof(hX170dwl_t));

    if(dec_dwl == NULL)
    {
        DWL_DEBUG("Init: failed to allocate an instance\n");
        return NULL;
    }

#ifdef INTERNAL_TEST
    dec_dwl->regDump = FS_FOpen(REG_DUMP_FILE, "a+");
    if(NULL == dec_dwl->regDump)
    {
        DWL_DEBUG("DWL: failed to open: %s\n", REG_DUMP_FILE);
        goto err;
    }
#endif

    //pthread_mutex_lock(&x170_init_mutex);
	 OS_EnterRegion ();
	 
    dec_dwl->clientType = param->clientType;

    switch (dec_dwl->clientType)
    {
    case DWL_CLIENT_TYPE_H264_DEC:
    case DWL_CLIENT_TYPE_MPEG4_DEC:
    case DWL_CLIENT_TYPE_JPEG_DEC:
    case DWL_CLIENT_TYPE_PP:
    case DWL_CLIENT_TYPE_VC1_DEC:
    case DWL_CLIENT_TYPE_MPEG2_DEC:
    case DWL_CLIENT_TYPE_VP6_DEC:
    case DWL_CLIENT_TYPE_VP8_DEC:
    case DWL_CLIENT_TYPE_RV_DEC:
    case DWL_CLIENT_TYPE_AVS_DEC:
        break;
    default:
        DWL_DEBUG("DWL: Unknown client type no. %d\n", dec_dwl->clientType);
        goto err;
    }

    //dec_dwl->fd = -1;
    //dec_dwl->fd_mem = -1;
    //dec_dwl->fd_memalloc = -1;
    dec_dwl->pRegBase = NULL;
    //dec_dwl->sigio_needed = 0;

    /* Linear momories not needed in pp */
    if(dec_dwl->clientType != DWL_CLIENT_TYPE_PP)
    {
        /* open memalloc for linear memory allocation */
    }

    /* open mem device for memory mapping */
	 base = HXDEC_LOGIC_MODULE0_BASE;
	 dec_dwl->regSize = DEC_IO_SIZE;

    /* map the hw registers to user space */
    dec_dwl->pRegBase = (volatile u32 *)base;

    DWL_DEBUG("DWL: regs size %d bytes, virt %08x\n", dec_dwl->regSize,
              (u32) dec_dwl->pRegBase);

#if 0
    {
        /* use ASIC ID as the shared sem key */
        key_t key = X170_SEM_KEY;
        int semid;

        if((semid = binary_semaphore_allocation(key, O_RDWR)) != -1)
        {
            DWL_DEBUG("DWL: HW lock sem %x aquired\n", key);
        }
        else if(errno == ENOENT)
        {
            semid = binary_semaphore_allocation(key, IPC_CREAT | O_RDWR);

            binary_semaphore_initialize(semid);

            DWL_DEBUG("DWL: HW lock sem %x created\n", key);
        }
        else
        {
            DWL_DEBUG("DWL: FAILED to get HW lock sem %x\n", key);
            goto err;
        }

        dec_dwl->semid = semid;
    }
#endif

    // pthread_mutex_unlock(&x170_init_mutex);
	 OS_LeaveRegion ();
    return dec_dwl;

  err:
    // pthread_mutex_unlock(&x170_init_mutex);
	 OS_LeaveRegion (); 
    DWLRelease(dec_dwl);
    return NULL;
}

/*------------------------------------------------------------------------------
    Function name   : DWLRelease
    Description     : Release a DWl instance

    Return type     : i32 - 0 for success or a negative error code

    Argument        : const void * instance - instance to be released
------------------------------------------------------------------------------*/
i32 DWLRelease(const void *instance)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;

    assert(dec_dwl != NULL);
    if(dec_dwl == NULL)
    {
		 return DWL_ERROR;
	 }

    // pthread_mutex_lock(&x170_init_mutex);
	 OS_EnterRegion ();

    //if(dec_dwl->pRegBase != MAP_FAILED)
     //   DWLUnmapRegisters((void*)dec_dwl->pRegBase, dec_dwl->regSize);

    dec_dwl->pRegBase = NULL;

    //if(dec_dwl->fd != -1)
    //    close(dec_dwl->fd);

    //if(dec_dwl->fd_mem != -1)
    //    close(dec_dwl->fd_mem);

    /* linear memory allocator */
    //if(dec_dwl->fd_memalloc != -1)
    //    close(dec_dwl->fd_memalloc);

#ifdef INTERNAL_TEST
    fclose(dec_dwl->regDump);
    dec_dwl->regDump = NULL;
#endif

    kernel_free(dec_dwl);

    //pthread_mutex_unlock(&x170_init_mutex);
	 OS_LeaveRegion (); 
    return (DWL_OK);
}

/*------------------------------------------------------------------------------
    Function name   : DWLWaitDecHwReady
    Description     : Wait until hardware has stopped running.
                      Used for synchronizing software runs with the hardware.
                      The wait can succeed or timeout.

    Return type     : i32 - one of the values DWL_HW_WAIT_OK
                                              DWL_HW_WAIT_TIMEOUT

    Argument        : const void * instance - DWL instance
                      u32 timeout - timeout period for the wait specified in
                                milliseconds; 0 will perform a poll of the
                                hardware status and -1 means an infinit wait
------------------------------------------------------------------------------*/

i32 DWLWaitDecHwReady(const void *instance, u32 timeout)
{
    return DWLWaitPpHwReady(instance, timeout);
}

/*------------------------------------------------------------------------------
    Function name   : DWLWaitHwReady
    Description     : Wait until hardware has stopped running.
                      Used for synchronizing software runs with the hardware.
                      The wait can succeed or timeout.

    Return type     : i32 - one of the values DWL_HW_WAIT_OK
                                              DWL_HW_WAIT_TIMEOUT

    Argument        : const void * instance - DWL instance
                      u32 timeout - timeout period for the wait specified in
                                milliseconds; 0 will perform a poll of the
                                hardware status and -1 means an infinit wait
------------------------------------------------------------------------------*/

i32 DWLWaitPpHwReady(const void *instance, u32 timeout)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;
    volatile u32 irq_stats;

    u32 irqRegOffset;

    assert(dec_dwl != NULL);

    if(dec_dwl->clientType == DWL_CLIENT_TYPE_PP)
        irqRegOffset = HX170PP_REG_START;   /* pp ctrl reg offset */
    else
        irqRegOffset = HX170DEC_REG_START;  /* decoder ctrl reg offset */

    /* wait for decoder */
#ifdef _READ_DEBUG_REGS
    (void) DWLReadReg(dec_dwl, 40 * 4);
    (void) DWLReadReg(dec_dwl, 41 * 4);
#endif
	 

#ifdef _ARKN141_CODEC_DECODE_INTERRUPT_
	 
	 if(timeout == 0)
	 {
    	irq_stats = DWLReadReg(dec_dwl, irqRegOffset);
    	irq_stats = (irq_stats >> 12) & 0xFF;
		if(irq_stats != 0)
			return DWL_HW_WAIT_OK;
		else
			return DWL_HW_WAIT_TIMEOUT;
	 }
	 
	 switch(arkn141_codec_waiting_for_decode_finished(timeout))
	 {
		 case 0:	 
			 return DWL_HW_WAIT_OK;
			 
	    case -2:	 
			 return DWL_HW_WAIT_TIMEOUT;

		 case -1:
		 default:
			 return DWL_HW_WAIT_ERROR;
	 }
		 
#else
	 
    irq_stats = DWLReadReg(dec_dwl, irqRegOffset);
    irq_stats = (irq_stats >> 12) & 0xFF;
    if(irq_stats != 0)
    {
        return DWL_HW_WAIT_OK;
    }
	 else if(timeout)
	 {
		 //  -1 means an infinit wait
		 unsigned int stop_ticket = 0;
		 i32 ret = DWL_HW_WAIT_TIMEOUT;
		 if(timeout != (u32)(-1))
		 {
			 // 设置超时时限
			 stop_ticket = timeout + XM_GetTickCount();
		 }
		 
		 do
		 {
#ifdef _READ_DEBUG_REGS
			(void) DWLReadReg(dec_dwl, 40 * 4);
			(void) DWLReadReg(dec_dwl, 41 * 4);
			(void) DWLReadReg(dec_dwl, 42 * 4);
			(void) DWLReadReg(dec_dwl, 43 * 4);
#endif

			irq_stats = DWLReadReg(dec_dwl, irqRegOffset);
			irq_stats = (irq_stats >> 12) & 0xFF;

         if(irq_stats != 0)
         {
            ret = DWL_HW_WAIT_OK;
            break;
         }
			
			if(stop_ticket && XM_GetTickCount() >= stop_ticket)
			{
				// 超时
				ret = DWL_HW_WAIT_TIMEOUT;
				break;
			}
				
			OS_Delay (1);
		 } while (1);
		 
		 return ret;
	 }
	 else
	 {
		// pooling模式
		 return DWL_HW_WAIT_TIMEOUT;
	 }
#endif	// 	_ARKN141_CODEC_DECODE_INTERRUPT_	 
}

