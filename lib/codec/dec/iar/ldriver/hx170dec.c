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
--  Abstract : x170 Decoder device driver (kernel module)
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: hx170dec.c,v $
--  $Date: 2010/02/08 14:08:00 $
--  $Revision: 1.12 $
--
------------------------------------------------------------------------------*/

//#include <linux/kernel.h>
//#include <linux/module.h>
/* needed for __init,__exit directives */
//#include <linux/init.h>
/* needed for remap_pfn_range
	SetPageReserved
	ClearPageReserved
*/
//#include <linux/mm.h>
/* obviously, for kmalloc */
//#include <linux/slab.h>
/* for struct file_operations, register_chrdev() */
//#include <linux/fs.h>
/* standard error codes */
//#include <linux/errno.h>

//#include <linux/moduleparam.h>
/* request_irq(), free_irq() */
//#include <linux/interrupt.h>

/* needed for virt_to_phys() */
//#include <asm/io.h>
//#include <linux/pci.h>
//#include <asm/uaccess.h>
//#include <linux/ioport.h>

//#include <asm/irq.h>

//#include <linux/version.h>

/* our own stuff */
#include <stdlib.h>
#include <stdio.h>
#include "basetype.h"
#include "hx170dec.h"
#include "irqs.h"
#include "arkn141_codec.h"

/* module description */
//MODULE_LICENSE("Proprietary");
//MODULE_AUTHOR("Hantro Products Oy");
//MODULE_DESCRIPTION("driver module for 8170/81990 Hantro decoder/pp");

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
#define DEC_IRQ                     CODEC_DECODER_INT

#define HX_DEC_INTERRUPT_BIT        0x100
#define HX_PP_INTERRUPT_BIT         0x100

//#define	printk	printf
#define	printk(...)	
void writel (unsigned int value, volatile unsigned char *address);
unsigned int readl (volatile unsigned char *address);



static const int DecHwId[] = { 0x8190, 0x8170, 0x9170, 0x9190, 0x6731 };

//static u32 hx_pp_instance = 0;
//static u32 hx_dec_instance = 0;

static unsigned long base_port = HXDEC_LOGIC_MODULE0_BASE;
static int irq = DEC_IRQ;

#define	KERN_INFO

/* module_param(name, type, perm) */
//module_param(base_port, ulong, 0);
//module_param(irq, int, 0);

/* and this is our MAJOR; use 0 for dynamic allocation (recommended)*/
//static int hx170dec_major = 0;

/* here's all the must remember stuff */
typedef struct
{
    char *buffer;
    unsigned long iobaseaddr;
    unsigned int iosize;
    volatile u8 *hwregs;
    int irq;
    //struct fasync_struct *async_queue_dec;
    //struct fasync_struct *async_queue_pp;
} hx170dec_t;

static hx170dec_t hx170dec_data;    /* dynamic allocation? */

#ifdef HW_PERFORMANCE
//static struct timeval end_time;
#endif

static int ReserveIO(void);
static void ReleaseIO(void);

static void ResetAsic(hx170dec_t * dev);

#ifdef HX170DEC_DEBUG
static void dump_regs(unsigned long data);
#endif

/* IRQ handler */
static void hx170dec_isr(void);



/*------------------------------------------------------------------------------
    Function name   : hx170dec_init
    Description     : Initialize the driver

    Return type     : int
------------------------------------------------------------------------------*/

int  hx170dec_init(void)
{
    int result;

    PDEBUG("module init\n");

    printk(KERN_INFO "hx170dec: dec/pp kernel module. %s \n", "$Revision: 1.12 $");
    printk(KERN_INFO "hx170dec: supports 8170 and 8190 hardware \n");
    printk(KERN_INFO "hx170dec: base_port=0x%08lx irq=%i\n", base_port, irq);

    hx170dec_data.iobaseaddr = base_port;
    hx170dec_data.iosize = DEC_IO_SIZE;
    hx170dec_data.irq = irq;



    result = ReserveIO();
    if(result < 0)
    {
        goto err;
    }

    ResetAsic(&hx170dec_data);  /* reset hardware */
    /* get the IRQ line */
#ifdef _ARKN141_CODEC_DECODE_INTERRUPT_
	 request_irq (irq, PRIORITY_ONE, hx170dec_isr);	
#else	 
	 if(0)
    //if(irq > 0)
    {
		 request_irq (irq, PRIORITY_ONE, hx170dec_isr);

    }
    else
    {
        printk(KERN_INFO "hx170dec: IRQ not in use!\n");
    }
#endif	 // _ARKN141_CODEC_DECODE_INTERRUPT_

    return 0;

  err:
    printk(KERN_INFO "hx170dec: module not inserted\n");
    return result;
}

/*------------------------------------------------------------------------------
    Function name   : hx170dec_cleanup
    Description     : clean up

    Return type     : int
------------------------------------------------------------------------------*/

void  hx170dec_cleanup(void)
{
    hx170dec_t *dev = (hx170dec_t *) & hx170dec_data;

    /* clear dec IRQ */
    writel(0, dev->hwregs + X170_INTERRUPT_REGISTER_DEC);
    /* clear pp IRQ */
    writel(0, dev->hwregs + X170_INTERRUPT_REGISTER_PP);

#ifdef HX170DEC_DEBUG
    dump_regs((unsigned long) dev); /* dump the regs */
#endif

    /* free the IRQ */
    if(dev->irq != -1)
    {
        //free_irq(dev->irq, (void *) dev);
    }

    ReleaseIO();

    printk(KERN_INFO "hx170dec: module removed\n");
    return;
}


static int CheckHwId(hx170dec_t * dev)
{
    long int hwid;

    size_t numHw = sizeof(DecHwId) / sizeof(*DecHwId);

    hwid = readl(dev->hwregs);
    printk( "hx170dec: HW ID=0x%08lx\n", hwid);

    hwid = (hwid >> 16) & 0xFFFF;   /* product version only */

    while(numHw--)
    {
        if(hwid == DecHwId[numHw])
        {
            printk( "hx170dec: Compatible HW found at 0x%08lx\n",
                   dev->iobaseaddr);
            return 1;
        }
    }

    printk( "hx170dec: No Compatible HW found at 0x%08lx\n",
           dev->iobaseaddr);
    return 0;
}

/*------------------------------------------------------------------------------
    Function name   : ReserveIO
    Description     : IO reserve

    Return type     : int
------------------------------------------------------------------------------*/
static int ReserveIO(void)
{

    hx170dec_data.hwregs =
        (volatile u8 *) hx170dec_data.iobaseaddr;

    if(hx170dec_data.hwregs == NULL)
    {
        printk( "hx170dec: failed to ioremap HW regs\n");
        ReleaseIO();
        return -1;
    }

    /* check for correct HW */
    if(!CheckHwId(&hx170dec_data))
    {
        ReleaseIO();
        return -1;
    }

    return 0;
}

/*------------------------------------------------------------------------------
    Function name   : releaseIO
    Description     : release

    Return type     : void
------------------------------------------------------------------------------*/

static void ReleaseIO(void)
{
	hx170dec_data.hwregs = NULL;
    //if(hx170dec_data.hwregs)
     //   iounmap((void *) hx170dec_data.hwregs);
    //release_mem_region(hx170dec_data.iobaseaddr, hx170dec_data.iosize);
}

/*------------------------------------------------------------------------------
    Function name   : hx170dec_isr
    Description     : interrupt handler

    Return type     : irqreturn_t
------------------------------------------------------------------------------*/
void hx170dec_isr(void)
{
    //unsigned int handled = 0;

    hx170dec_t *dev = (hx170dec_t *) &hx170dec_data;
    u32 irq_status_dec;
    u32 irq_status_pp;

    //handled = 0;

    /* interrupt status register read */
    irq_status_dec = readl(dev->hwregs + X170_INTERRUPT_REGISTER_DEC);
    irq_status_pp = readl(dev->hwregs + X170_INTERRUPT_REGISTER_PP);

    if((irq_status_dec & HX_DEC_INTERRUPT_BIT) ||
       (irq_status_pp & HX_PP_INTERRUPT_BIT))
    {

        if(irq_status_dec & HX_DEC_INTERRUPT_BIT)
        {
            /* clear dec IRQ */
            writel(irq_status_dec & (~HX_DEC_INTERRUPT_BIT),
                   dev->hwregs + X170_INTERRUPT_REGISTER_DEC);
            //printf("decoder IRQ received!\n");
				
#ifdef _ARKN141_CODEC_DECODE_INTERRUPT_
				arkn141_codec_trigger_decode_finish_event ();
#endif				
        }

        if(irq_status_pp & HX_PP_INTERRUPT_BIT)
        {
            /* clear pp IRQ */
            writel(irq_status_pp & (~HX_PP_INTERRUPT_BIT),
                   dev->hwregs + X170_INTERRUPT_REGISTER_PP);

            printf("pp IRQ received!\n");
        }

    }
    else
    {
        PDEBUG("IRQ received, but not x170's!\n");
    }

}

/*------------------------------------------------------------------------------
    Function name   : ResetAsic
    Description     : reset asic

    Return type     :
------------------------------------------------------------------------------*/

void ResetAsic(hx170dec_t * dev)
{
    int i;

    writel(0, dev->hwregs + 0x04);

    for(i = 4; i < dev->iosize; i += 4)
    {
        writel(0, dev->hwregs + i);
    }
}

/*------------------------------------------------------------------------------
    Function name   : dump_regs
    Description     : Dump registers

    Return type     :
------------------------------------------------------------------------------*/
#ifdef HX170DEC_DEBUG
void dump_regs(unsigned long data)
{
    hx170dec_t *dev = (hx170dec_t *) data;
    int i;

    PDEBUG("Reg Dump Start\n");
    for(i = 0; i < dev->iosize; i += 4)
    {
        PDEBUG("\toffset %02X = %08X\n", i, readl(dev->hwregs + i));
    }
    PDEBUG("Reg Dump End\n");
}
#endif
