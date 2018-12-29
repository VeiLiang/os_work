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
--  Abstract : 6280/7280/8270/8290 Encoder device driver (kernel module)
--
------------------------------------------------------------------------------*/

//#include <linux/kernel.h>
//#include <linux/module.h>
/* needed for __init,__exit directives */
//#include <linux/init.h>
/* needed for remap_page_range 
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

#include "hx280enc.h"
#include "irqs.h"
#include "arkn141_codec.h"

/* module description */
//MODULE_LICENSE("Proprietary");
//MODULE_AUTHOR("Hantro Products Oy");
//MODULE_DESCRIPTION("Hantro 6280/7280/8270/8290 Encoder driver");

/* this is ARM Integrator specific stuff */
#define INTEGRATOR_LOGIC_MODULE0_BASE   0x70078000
/*
#define INTEGRATOR_LOGIC_MODULE1_BASE   0xD0000000
#define INTEGRATOR_LOGIC_MODULE2_BASE   0xE0000000
#define INTEGRATOR_LOGIC_MODULE3_BASE   0xF0000000
*/

//#define VP_PB_INT_LT                    1		// ÖÐ¶ÏºÅ
/*
#define INT_EXPINT1                     10
#define INT_EXPINT2                     11
#define INT_EXPINT3                     12
*/
/* these could be module params in the future */

#define ENC_IO_BASE                 INTEGRATOR_LOGIC_MODULE0_BASE
#define ENC_IO_SIZE                 (96 * 4)    /* bytes */

#define ENC_HW_ID1                  0x62800000
#define ENC_HW_ID2                  0x72800000
#define ENC_HW_ID3                  0x82700000
#define ENC_HW_ID4                  0x82900000

#define HX280ENC_BUF_SIZE           0

#define	printk	printf

void writel (unsigned int value, volatile unsigned char *address)
{
	*(volatile unsigned int *)address = value;
}

unsigned int readl (volatile unsigned char *address)
{
	return *(volatile unsigned int *)address;
}

unsigned long base_port = INTEGRATOR_LOGIC_MODULE0_BASE;
static const int irq = CODEC_ENCODER_INT;

/* and this is our MAJOR; use 0 for dynamic allocation (recommended)*/
static int hx280enc_major = 0;

/* here's all the must remember stuff */
typedef struct
{
    char *buffer;
    unsigned int buffsize;
    unsigned long iobaseaddr;
    unsigned int iosize;
    volatile unsigned char *hwregs;
    unsigned int irq;
    //struct fasync_struct *async_queue;
} hx280enc_t;

/* dynamic allocation? */
static hx280enc_t hx280enc_data;

static int ReserveIO(void);
static void ReleaseIO(void);
static void ResetAsic(hx280enc_t * dev);

#ifdef HX280ENC_DEBUG
static void dump_regs(unsigned long data);
#endif

#ifdef _ARKN141_H264_ENCODE_INTERRUPT_
/* IRQ handler */
static void hx280enc_isr(void);
#endif	// #ifdef _ARKN141_H264_ENCODE_INTERRUPT_





int  hx280enc_init(void)
{
    //printk("hx280enc: module init - base_port=0x%08lx irq=%i\n",
    //       base_port, irq);

    hx280enc_data.iobaseaddr = base_port;
    hx280enc_data.iosize = ENC_IO_SIZE;
    hx280enc_data.irq = irq;
    //hx280enc_data.async_queue = NULL;
    hx280enc_data.hwregs = NULL;

    ReserveIO();

    ResetAsic(&hx280enc_data);  /* reset hardware */

#ifdef _ARKN141_H264_ENCODE_INTERRUPT_
    /* get the IRQ line */
	 request_irq (irq, PRIORITY_ONE, hx280enc_isr);
#endif	 // #ifdef _ARKN141_H264_ENCODE_INTERRUPT_

	 
    PDEBUG("hx280enc: module inserted. Major <%d>\n", hx280enc_major);

    return 0;
}

void  hx280enc_exit(void)
{
    writel(0, hx280enc_data.hwregs + 0x38); /* disable HW */
    writel(0, hx280enc_data.hwregs + 0x04); /* clear enc IRQ */

    /* free the encoder IRQ */

    ReleaseIO();

    PDEBUG("hx280enc: module removed\n");
    return;
}


static int ReserveIO(void)
{
    long int hwid;

    hx280enc_data.hwregs = (volatile unsigned char *)hx280enc_data.iobaseaddr;

    hwid = readl(hx280enc_data.hwregs);

#if 1
    /* check for encoder HW ID */
    if((((hwid >> 16) & 0xFFFF) != ((ENC_HW_ID1 >> 16) & 0xFFFF)) &&
       (((hwid >> 16) & 0xFFFF) != ((ENC_HW_ID2 >> 16) & 0xFFFF)) &&
       (((hwid >> 16) & 0xFFFF) != ((ENC_HW_ID3 >> 16) & 0xFFFF)) &&
       (((hwid >> 16) & 0xFFFF) != ((ENC_HW_ID4 >> 16) & 0xFFFF)))
    {
        printk("hx280enc: HW not found at 0x%08lx\n",
               hx280enc_data.iobaseaddr);
#ifdef HX280ENC_DEBUG
        dump_regs((unsigned long) &hx280enc_data);
#endif
        ReleaseIO();
        return -1;
    }
#endif
	 
    PDEBUG(
           "hx280enc: HW at base <0x%08lx> with ID <0x%08lx>\n",
           hx280enc_data.iobaseaddr, hwid);
    return 0;
}

static void ReleaseIO(void)
{
}

#ifdef _ARKN141_H264_ENCODE_INTERRUPT_
void hx280enc_isr(void)
{
    hx280enc_t *dev = (hx280enc_t *) &hx280enc_data;
    u32 irq_status;
	 
	 //printf ("hx280enc_isr\n");

    irq_status = readl(dev->hwregs + 0x04);

    if(irq_status & 0x01)
    {

        /* clear enc IRQ and slice ready interrupt bit */
        writel(irq_status & (~0x101), dev->hwregs + 0x04);
		  
		  arkn141_codec_trigger_h264_encode_finish_event ();

        /* Handle slice ready interrupts. The reference implementation
         * doesn't signal slice ready interrupts to EWL.
         * The EWL will poll the slices ready register value. */
        if ((irq_status & 0x1FE) == 0x100)
        {
            PDEBUG("Slice ready IRQ handled!\n");
            return ;
        }

        /* All other interrupts will be signaled to EWL. */

        PDEBUG("IRQ handled!\n");
        return;
    }
    else
    {
        PDEBUG("IRQ received, but NOT handled!\n");
        return;
    }

}
#endif // #ifdef _ARKN141_H264_ENCODE_INTERRUPT_


void ResetAsic(hx280enc_t * dev)
{
    int i;

    writel(0, dev->hwregs + 0x38);

    for(i = 4; i < dev->iosize; i += 4)
    {
        writel(0, dev->hwregs + i);
    }
}

#ifdef HX280ENC_DEBUG
void dump_regs(unsigned long data)
{
    hx280enc_t *dev = (hx280enc_t *) data;
    int i;

    PDEBUG("Reg Dump Start\n");
    for(i = 0; i < dev->iosize; i += 4)
    {
        PDEBUG("\toffset %02X = %08X\n", i, readl(dev->hwregs + i));
    }
    PDEBUG("Reg Dump End\n");
}
#endif
