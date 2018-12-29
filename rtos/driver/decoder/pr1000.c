#ifdef __LINUX_SYSTEM__
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#ifndef CONFIG_HISI_SNAPSHOT_BOOT
#include <linux/miscdevice.h>
#endif // CONFIG_HISI_SNAPSHOT_BOOT
#include <linux/kthread.h>

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/system.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/string.h>
#include <linux/list.h>
#include <asm/delay.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/poll.h>
#include <asm/bitops.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <linux/moduleparam.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>

#include "pr1000.h"
#include "pr1000_func.h"
#include "pr1000_ceqfunc.h"
#include "drv_cq.h"
#include "pr1000_user_config.h"
#include "pr1000_drvcommon.h"
#include "pr1000_ptz_drvtable.h"

#ifdef CONFIG_HISI_SNAPSHOT_BOOT
#include "himedia.h"
#define DEV_NAME "pr1000"
static struct himedia_device himedia_pr1000Device;
#endif // CONFIG_HISI_SNAPSHOT_BOOT

static struct task_struct *gpKthreadId = NULL;
extern int PR1000_Kthread(void *arg);

#else //#ifdef __LINUX_SYSTEM__

#include <stdio.h>
#include <stdlib.h>

#include "pr1000.h"
#include "pr1000_func.h"
#include "pr1000_ceqfunc.h"
#include "drv_cq.h"
#include "pr1000_user_config.h"
#include "pr1000_drvcommon.h"
#include "pr1000_ptz_drvtable.h"

#endif // __LINUX_SYSTEM__

/* ############################################ */
/*! global variable prototype */
/* ############################################ */

_drvHost gDrvHost;
_drvHost *gpDrvHost = NULL;

/* ############################################ */
/*! function prototype */
/* ############################################ */
/*! proc function */
#ifdef SUPPORT_PROC_SYSTEM //support proc system for linux debugging.

struct proc_dir_entry *gpPr1000RootProcDir = NULL;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
static ssize_t Pr1000ProcWrRegDumpFunc(struct file *pFile, const char __user *pBuffer, size_t count, loff_t *pData)
#else
static int Pr1000ProcWrRegDumpFunc(struct file *pFile, const char *pBuffer, unsigned long count, void *pData)
#endif //LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)
{
	const int fd = 0; //Normal:0, Isr:-1
	int len;
	char *pRealdata;
	uint8_t read_value = 0;

	char devStrSlv[5];
	char regStrPage[5];
	char regStrAddr[5];
	char regStrLen[10];

	uint8_t devSlv = 0;
	uint8_t regPage = 0;
	uint8_t regAddr = 0;
	int regLen = 0;

	pRealdata = (char *)pData;

	if(copy_from_user(pRealdata, pBuffer, count)) 
	{
		return -EFAULT;
	}

	pRealdata[count] = '\0';
	len = strlen(pRealdata);
	if(pRealdata[len-1] == '\n') pRealdata[--len] = 0;

	/* convert reg addr length */
	if( (sscanf(pRealdata, "%s %s %s %s", devStrSlv, regStrPage, regStrAddr, regStrLen) != 4) )
	{
		printk("usage: echo \"SlvAddr Page Addr Len\" > /proc/pr1000/reg_dump\n");
		return(count);
	}
	Dbg("str:%s %s %s %s\n", devStrSlv COMMA regStrPage COMMA regStrAddr COMMA regStrLen);

	devSlv = simple_strtoul(devStrSlv, NULL, 16);
	regPage = simple_strtoul(regStrPage, NULL, 16);
	regAddr = simple_strtoul(regStrAddr, NULL, 16);
	regLen = simple_strtoul(regStrLen, NULL, 16);
	Dbg("0x%02x: 0x%02x 0x%02x 0x%02x\n", devSlv COMMA regPage COMMA regAddr COMMA regLen);

	for(len = 0; len < regLen; len++)
	{
		PR1000_PageRead(fd, devSlv<<1, regPage, regAddr, &read_value);
		Print("RegDump: 0x%02x 0x%02x 0x%02x:[0x%02x]\n", devSlv COMMA regPage COMMA regAddr COMMA read_value);
		regAddr++;
	}

	return(count);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
static ssize_t Pr1000ProcRdRegDumpFunc(struct file *pFile, char __user *buffer, size_t size, loff_t *dat)
#else
static int Pr1000ProcRdRegDumpFunc(char *buffer, char **start, off_t offset, int count, int *peof, void *dat)
#endif //LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)
{
	printk("usage: echo \"SlvAddr Page Addr Len\" > /proc/pr1000/reg_dump\n");
	PrintString("usage: echo \"SlvAddr Page Addr Len\" > /proc/pr1000/reg_dump\n");

	printk("---- reg_dump shell script example ------\n");
	printk("#!/bin/sh\n\n");
	printk("if [ $# -ne 5 ] && [ $# -ne 4 ]; then\n");
	printk(" echo \"usage: $0 [r|w] [slv] [page] [reg] [data]\"\n");
	printk(" exit\n");
	printk("fi\n");
	printk("RW=$1 SLVADDR=$2 PAGENUM=$3 REGADDR=$4 DATA=$5\n");
	printk("if [ $RW == 'r' ]; then\n");
	printk(" if [ $# -eq 4 ]; then\n");
	printk("  DATA=1\n");
	printk(" fi\n");
	printk(" echo \"0x$SLVADDR $PAGENUM 0x$REGADDR 0x$DATA\" > /proc/pr1000/reg_dump\n");
	printk(" let \"DATA=DATA+1\"\n");
	printk(" if [ -e /usr/bin/tail ]; then\n");
	printk("  tail -$DATA /proc/pr1000/log\n");
	printk(" else\n");
	printk("  cat /proc/pr1000/log\n");
	printk(" fi\n");
	printk("elif [ $RW == 'w' ]; then\n");
	printk(" if [ $# -eq 4 ]; then\n");
	printk("  DATA=1\n");
	printk("  echo \"0x$SLVADDR $PAGENUM 0x$REGADDR 0x$DATA\" > /proc/pr1000/reg_dump\n");
	printk("  let \"DATA=DATA+1\"\n");
	printk("  if [ -e /usr/bin/tail ]; then\n");
	printk("   tail -$DATA /proc/pr1000/log\n");
	printk("  else\n");
	printk("   cat /proc/pr1000/log\n");
	printk("  fi\n");
	printk(" else\n");
	printk("  echo \"0x$SLVADDR $PAGENUM 0x$REGADDR 0x$DATA\" > /proc/pr1000/reg_write\n");
	printk("  if [ -e /usr/bin/tail ]; then\n");
	printk("   tail -2 /proc/pr1000/log\n");
	printk("  else\n");
	printk("   cat /proc/pr1000/log\n");
	printk("  fi\n");
	printk(" fi\n");
	printk("else \n");
	printk(" echo \"invalid. usage: $0 [r|w] [slv] [page] [reg] [data]\"\n");
	printk("fi\n");
	printk("exit\n");

	PrintString("---- reg_dump shell script example ------\n");
	PrintString("#!/bin/sh\n\n");
	PrintString("if [ $# -ne 5 ] && [ $# -ne 4 ]; then\n");
	PrintString(" echo \"usage: $0 [r|w] [slv] [page] [reg] [data]\"\n");
	PrintString(" exit\n");
	PrintString("fi\n");
	PrintString("RW=$1 SLVADDR=$2 PAGENUM=$3 REGADDR=$4 DATA=$5\n");
	PrintString("if [ $RW == 'r' ]; then\n");
	PrintString(" if [ $# -eq 4 ]; then\n");
	PrintString("  DATA=1\n");
	PrintString(" fi\n");
	PrintString(" echo \"0x$SLVADDR $PAGENUM 0x$REGADDR 0x$DATA\" > /proc/pr1000/reg_dump\n");
	PrintString(" let \"DATA=DATA+1\"\n");
	PrintString(" if [ -e /usr/bin/tail ]; then\n");
	PrintString("  tail -$DATA /proc/pr1000/log\n");
	PrintString(" else\n");
	PrintString("  cat /proc/pr1000/log\n");
	PrintString(" fi\n");
	PrintString("elif [ $RW == 'w' ]; then\n");
	PrintString(" if [ $# -eq 4 ]; then\n");
	PrintString("  DATA=1\n");
	PrintString("  echo \"0x$SLVADDR $PAGENUM 0x$REGADDR 0x$DATA\" > /proc/pr1000/reg_dump\n");
	PrintString("  let \"DATA=DATA+1\"\n");
	PrintString("  if [ -e /usr/bin/tail ]; then\n");
	PrintString("   tail -$DATA /proc/pr1000/log\n");
	PrintString("  else\n");
	PrintString("   cat /proc/pr1000/log\n");
	PrintString("  fi\n");
	PrintString(" else\n");
	PrintString("  echo \"0x$SLVADDR $PAGENUM 0x$REGADDR 0x$DATA\" > /proc/pr1000/reg_write\n");
	PrintString("  if [ -e /usr/bin/tail ]; then\n");
	PrintString("   tail -2 /proc/pr1000/log\n");
	PrintString("  else\n");
	PrintString("   cat /proc/pr1000/log\n");
	PrintString("  fi\n");
	PrintString(" fi\n");
	PrintString("else \n");
	PrintString(" echo \"invalid. usage: $0 [r|w] [slv] [page] [reg] [data]\"\n");
	PrintString("fi\n");
	PrintString("exit\n");
	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
static ssize_t Pr1000ProcWrRegWriteFunc(struct file *pFile, const char __user *pBuffer, size_t count, loff_t *pData)
#else
static int Pr1000ProcWrRegWriteFunc(struct file *pFile, const char *pBuffer, unsigned long count, void *pData)
#endif //LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)
{
	int len;
	char *pRealdata;
	const int fd = 0; //Normal:0, Isr:-1

	char devStrSlv[5];
	char regStrPage[5];
	char regStrAddr[5];
	char regStrData[5];

	uint8_t devSlv = 0;
	uint8_t regPage = 0;
	uint8_t regAddr = 0;
	uint8_t regData = 0;

	pRealdata = (char *)pData;

	Dbg("count:0x%lx\n", count);
	if(copy_from_user(pRealdata, pBuffer, count))
	{
		return -EFAULT;
	}

	pRealdata[count] = '\0';
	len = strlen(pRealdata);
	Dbg("len:0x%x\n", len);
	if(pRealdata[len-1] == '\n') pRealdata[--len] = 0;

	/* convert reg addr length */
	if( (sscanf(pRealdata, "%s %s %s %s", devStrSlv, regStrPage, regStrAddr, regStrData) != 4) )
	{
		printk("usage: echo \"SlvAddr Page Addr Data\" > /proc/pr1000/reg_write\n");
		return(count);
	}
	Dbg("str:%s %s %s %s\n", devStrSlv COMMA regStrPage COMMA regStrAddr COMMA regStrData);

	devSlv = simple_strtoul(devStrSlv, NULL, 16);
	regPage = simple_strtoul(regStrPage, NULL, 16);
	regAddr = simple_strtoul(regStrAddr, NULL, 16);
	regData = simple_strtoul(regStrData, NULL, 16);
	Dbg("0x%02x: 0x%02x 0x%02x 0x%02x\n", devSlv COMMA regPage COMMA regAddr COMMA regData);

	PR1000_PageWrite(fd, devSlv<<1, regPage, regAddr, regData);
	Print("RegWrite: 0x%02x 0x%02x 0x%02x 0x%02x\n", devSlv COMMA regPage COMMA regAddr COMMA regData);

	return(count);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
static ssize_t Pr1000ProcRdRegWriteFunc(struct file *pFile, char __user *buffer, size_t size, loff_t *dat)
#else
static int Pr1000ProcRdRegWriteFunc(char *buffer, char **start, off_t offset, int count, int *peof, void *dat)
#endif //LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)
{
	printk("usage: echo \"SlvAddr Page Addr Data\" > /proc/pr1000/reg_write\n");
	PrintString("usage: echo \"SlvAddr Page Addr Data\" > /proc/pr1000/reg_write\n");

	printk("---- reg_write shell script example ------\n");
	printk("#!/bin/sh\n\n");
	printk("if [ $# -ne 5 ]; then\n");
	printk(" echo \"usage: $0 [r|w] [slv] [page] [reg] [data]\"\n");
	printk(" exit\n");
	printk("fi\n");
	printk("RW=$1 SLVADDR=$2 PAGENUM=$3 REGADDR=$4 DATA=$5\n");
	printk("if [ $RW == 'r' ]; then\n");
	printk(" echo \"0x$SLVADDR $PAGENUM 0x$REGADDR 0x$DATA\" > /proc/pr1000/reg_dump\n");
	printk(" let \"DATA=DATA+1\"\n");
	printk(" tail -$DATA /proc/pr1000/log\n");
	printk("elif [ $RW == 'w' ]; then\n");
	printk(" echo \"0x$SLVADDR $PAGENUM 0x$REGADDR 0x$DATA\" > /proc/pr1000/reg_write\n");
	printk(" tail -2 /proc/pr1000/log\n");
	printk("else \n");
	printk(" echo \"invalid. usage: $0 [r|w] [slv] [page] [reg] [data]\"\n");
	printk("fi\n");
	printk("exit\n");
	PrintString("---- reg_write shell script example ------\n");
	PrintString("#!/bin/sh\n\n");
	PrintString("if [ $# -ne 5 ]; then\n");
	PrintString(" echo \"usage: $0 [r|w] [slv] [page] [reg] [data]\"\n");
	PrintString(" exit\n");
	PrintString("fi\n");
	PrintString("RW=$1 SLVADDR=$2 PAGENUM=$3 REGADDR=$4 DATA=$5\n");
	PrintString("if [ $RW == 'r' ]; then\n");
	PrintString(" echo \"0x$SLVADDR $PAGENUM 0x$REGADDR 0x$DATA\" > /proc/pr1000/reg_dump\n");
	PrintString(" let \"DATA=DATA+1\"\n");
	PrintString(" tail -$DATA /proc/pr1000/log\n");
	PrintString("elif [ $RW == 'w' ]; then\n");
	PrintString(" echo \"0x$SLVADDR $PAGENUM 0x$REGADDR 0x$DATA\" > /proc/pr1000/reg_write\n");
	PrintString(" tail -2 /proc/pr1000/log\n");
	PrintString("else \n");
	PrintString(" echo \"invalid. usage: $0 [r|w] [slv] [page] [reg] [data]\"\n");
	PrintString("fi\n");
	PrintString("exit\n");

	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
static const struct file_operations fopsRegDump = { 
        .read = Pr1000ProcRdRegDumpFunc,
        .write = Pr1000ProcWrRegDumpFunc,
        .llseek = default_llseek,
};
static const struct file_operations fopsRegWrite = { 
        .read = Pr1000ProcRdRegWriteFunc,
        .write = Pr1000ProcWrRegWriteFunc,
        .llseek = default_llseek,
};
#endif //LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)


////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int Pr1000SeqLogShow(struct seq_file *s, void *v)
{
	_drvHost *pHost = (_drvHost *)s->private;
	int logNum = 0, i = 0, ch = 0;

	//loff_t *spos = (loff_t *) v;

	seq_printf( s, "### PR1000 log info(Driver Version : v%s) ### \n", _VER);
	seq_printf( s, "--------------------------------------------------------------\n");
	logNum = ((pHost->drvLog.logMemNum) + 1) % DRV_LOG_MEM_NUM;
	for(i = 0; i < DRV_LOG_MEM_NUM; i++)
	{
		ch = 0;
		for(ch = 0; ch < pHost->drvLog.wrPos[logNum]; ch++)
		{
			seq_printf( s, "%c", pHost->drvLog.pMem[logNum][ch]);
		}
		logNum++;
		logNum %= DRV_LOG_MEM_NUM;
	}
	seq_printf( s, "--------------------------------------------------------------\n");

	return 0;
}

static int Pr1000ProcLogOpen(struct inode *inode, struct file *file)
{
	int err;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
	_drvHost *pHost = PDE_DATA(inode);
#else
	_drvHost *pHost = PDE(inode)->data;
#endif //LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)

	if(!try_module_get(THIS_MODULE))
		return -ENODEV;

	err = single_open(file, Pr1000SeqLogShow, pHost);
	if(err)
		module_put(THIS_MODULE);
	return err;
};

static int Pr1000ProcLogRelease(struct inode *inode, struct file *file)
{
        int res = single_release(inode, file);
        module_put(THIS_MODULE);
        return res;
}

static const struct file_operations pr1000ProcLogFileOps = {
	.owner   = THIS_MODULE,
	.open    = Pr1000ProcLogOpen,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = Pr1000ProcLogRelease,
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int Pr1000SeqShow(struct seq_file *s, void *v)
{
	_drvHost *pHost = (_drvHost *)s->private;
	int i, j;
	const int fd = 0; //Normal:0, Isr:-1

#ifndef DONT_SUPPORT_ETC_FUNC
	_stPrPwDown stPrPwDown;
#endif // DONT_SUPPORT_ETC_FUNC
	_stChnAttr stChnAttr;

#ifndef DONT_SUPPORT_VID_ENHANCEMENT
	_stCscAttr stCscAttr;
	_stContrast stContrast;
	_stBright stBright;
	_stSaturation stSaturation;
	_stHue stHue;
	_stSharpness stSharpness;
#endif // DONT_SUPPORT_VID_ENHANCEMENT

	_stNovidAttr stNovidAttr;
#ifndef DONT_SUPPORT_EVENT_FUNC
	_stMdAttr stMdAttr;
	_stMdLvSens stMdLvSens;
	_stMdSpSens stMdSpSens;
	_stMdTmpSens stMdTmpSens;
	_stMdVelocity stMdVelocity;
	_stBdAttr stBdAttr;
	_stBdLvSens stBdLvSens;
	_stBdSpSens stBdSpSens;
	_stBdTmpSens stBdTmpSens;
	_stNdAttr stNdAttr;
	_stNdLvSens stNdLvSens;
	_stNdTmpSens stNdTmpSens;
	_stDdAttr stDdAttr;
	_stDdLvSens stDdLvSens;
	_stDdTmpSens stDdTmpSens;
	_stDfdAttr stDfdAttr;
	_stDfdLvSens stDfdLvSens;
	_stDfdSpSens stDfdSpSens;
	_stDfdTmpSens stDfdTmpSens;

	_stMaskCellAttr stMaskCellAttr;
#endif // DONT_SUPPORT_EVENT_FUNC

#ifndef DONT_SUPPORT_AUD_ALINK
	_stAUDCascadeAttr stAUDCascadeAttr;
	_stAUDRecAttr stAUDRecAttr;
	_stAUDPbAttr stAUDPbAttr;
#endif // DONT_SUPPORT_AUD_ALINK

	_stIRQReg stIRQStatus;

	uint8_t u8Data[8] = {0, };

#ifndef DONT_SUPPORT_PTZ_FUNC
	_stPTZRxAttr stPTZRxAttr;
	_stPTZHVStartAttr stPTZHVStartAttr;
	_stPTZTxAttr stPTZTxAttr;
#endif // DONT_SUPPORT_PTZ_FUNC

	int (*pPortChSel)[4][2];
	uint8_t i2cReg = 0;
	uint8_t status;
	uint8_t ch; 
	_stCeqDet stCeqDet;
	_stManEQMan stManEQMan;
	uint8_t reg[0x14] = {0, };

	//loff_t *spos = (loff_t *) v;

	seq_printf( s, "### PR1000 info(Driver Version : v%s) ### \n", _VER);
 	seq_printf( s, "Driver Irq : %d(%s)\n", pHost->irq, (pHost->irq > 0)?"IRQ":"KTHREAD" );

 	seq_printf( s, "---------- Driver loading environment(pr1000_user_config.c) param ------------------\n");
	seq_printf( s, "[ENV] CHIP COUNT: %d\n", PR1000_CHIP_COUNT);
	seq_printf( s, "[ENV] I2C Slave addr: ");
	for(i = 0; i < PR1000_CHIP_COUNT; i++)
	{
		seq_printf( s, "0x%X ", PR1000_I2C_SLVADDRS[i]>>1);
	}
	seq_printf( s, "\n");
#ifndef DONT_SUPPORT_HELP_STRING 
	seq_printf( s, "[ENV] Default init format: [%s, %s]\n", _STR_PR1000_FORMAT[DEFAULT_INIT_FORMAT], _STR_PR1000_OUTRESOL[DEFAULT_INIT_RESOLUTION]);
#else // DONT_SUPPORT_HELP_STRING 
	seq_printf( s, "[ENV] Default init format: [%d, %d]\n", DEFAULT_INIT_FORMAT, DEFAULT_INIT_RESOLUTION);
#endif // DONT_SUPPORT_HELP_STRING 
	seq_printf( s, "[ENV] DRV_INT_SYNC_MSEC: %d\n", PR1000_INT_SYNC_PERIOD);

	seq_printf( s, "[ENV] Video output mux attribute.\n");
	seq_printf( s, "  VIDOUTF_MUX_CH_TYPE: %d (0:mux1ch,1:mux2ch,2:mux4ch)\n", pHost->sysHost.stOutFormatAttr.muxChCnt);
	seq_printf( s, "  VIDOUTF_16BIT_BUS: %d (0:8bit,1:16bit)\n", pHost->sysHost.stOutFormatAttr.b16bit);
	seq_printf( s, "  VIDOUTF_BT656: %d (0:16bit bt1120,1:8bit bt656)\n", pHost->sysHost.stOutFormatAttr.outfmt_bt656);
	seq_printf( s, "  VIDOUTF_DATARATE: %d (0:148.5/297M,1:148.5M,2:144M,3:108M)\n", pHost->sysHost.stOutFormatAttr.datarate);
	seq_printf( s, "  VIDOUTF_RESOLUTION: %d (0:1080p(720p60),1:720p,2:sd960,3:sd720)\n", pHost->sysHost.stOutFormatAttr.resol);
	seq_printf( s, "[ENV] AUDIO_ALINK_EN: %d (0:disable,1:enable)\n", PR1000_AUDIO_ALINK_EN);

	for(i = 0; i < PR1000_CHIP_COUNT; i++)
	{
		if(pHost->sysHost.stOutFormatAttr.muxChCnt == PR1000_VID_OUTF_MUX_1CH)
		{
			pPortChSel = (int (*)[4][2])&PR1000_VID_INOUT_MUX1CH_CHDEF[pHost->sysHost.stOutFormatAttr.b16bit][i];
		}
		else if(pHost->sysHost.stOutFormatAttr.muxChCnt == PR1000_VID_OUTF_MUX_2CH)
		{
			pPortChSel = (int (*)[4][2])&PR1000_VID_INOUT_MUX2CH_CHDEF[pHost->sysHost.stOutFormatAttr.b16bit][i];
		}
		else
		{
			pPortChSel = (int (*)[4][2])&PR1000_VID_INOUT_MUX4CH_CHDEF[pHost->sysHost.stOutFormatAttr.b16bit][i];
		}

		seq_printf( s, "  [CHIP%d]:\n", i);

		seq_printf( s, "    VIDOUTF_MUX_CHDEF_CH0_PORT: {%2d,%2d} | {%2d,%2d} | {%2d,%2d} | {%2d,%2d}\n", 
				pPortChSel[0][0][0], pPortChSel[0][0][1],
				pPortChSel[0][1][0], pPortChSel[0][1][1],
				pPortChSel[0][2][0], pPortChSel[0][2][1],
				pPortChSel[0][3][0], pPortChSel[0][3][1]);
		seq_printf( s, "    VIDOUTF_MUX_CHDEF_CH1_PORT: {%2d,%2d} | {%2d,%2d} | {%2d,%2d} | {%2d,%2d}\n", 
				pPortChSel[1][0][0], pPortChSel[1][0][1],
				pPortChSel[1][1][0], pPortChSel[1][1][1],
				pPortChSel[1][2][0], pPortChSel[1][2][1],
				pPortChSel[1][3][0], pPortChSel[1][3][1]);
		seq_printf( s, "    VIDOUTF_MUX_CHDEF_CH2_PORT: {%2d,%2d} | {%2d,%2d} | {%2d,%2d} | {%2d,%2d}\n", 
				pPortChSel[2][0][0], pPortChSel[2][0][1],
				pPortChSel[2][1][0], pPortChSel[2][1][1],
				pPortChSel[2][2][0], pPortChSel[2][2][1],
				pPortChSel[2][3][0], pPortChSel[2][3][1]);
		seq_printf( s, "    VIDOUTF_MUX_CHDEF_CH3_PORT: {%2d,%2d} | {%2d,%2d} | {%2d,%2d} | {%2d,%2d}\n", 
				pPortChSel[3][0][0], pPortChSel[3][0][1],
				pPortChSel[3][1][0], pPortChSel[3][1][1],
				pPortChSel[3][2][0], pPortChSel[3][2][1],
				pPortChSel[3][3][0], pPortChSel[3][3][1]);
		seq_printf( s, "    VIDOUTF_CLKPHASE: CH0:0x%x, CH1:0x%x, CH2:0x%x, CH3:0x%x\n", 
				PR1000_VIDOUTF_CLKPHASE[i][0], PR1000_VIDOUTF_CLKPHASE[i][1], PR1000_VIDOUTF_CLKPHASE[i][2], PR1000_VIDOUTF_CLKPHASE[i][3]);
		seq_printf( s, "    VIDOUTF_FORMAT_HD1080p: vdck_out_phase(0x%x)\n", 
				pr1000_reg_table_vidout_format[pHost->sysHost.stOutFormatAttr.muxChCnt][pHost->sysHost.stOutFormatAttr.b16bit][0].vdck_out_phase);
		seq_printf( s, "    VIDOUTF_FORMAT__HD720p: vdck_out_phase(0x%x)\n", 
				pr1000_reg_table_vidout_format[pHost->sysHost.stOutFormatAttr.muxChCnt][pHost->sysHost.stOutFormatAttr.b16bit][1].vdck_out_phase);
		seq_printf( s, "    VIDOUTF_FORMAT__SD960H: vdck_out_phase(0x%x)\n", 
				pr1000_reg_table_vidout_format[pHost->sysHost.stOutFormatAttr.muxChCnt][pHost->sysHost.stOutFormatAttr.b16bit][2].vdck_out_phase);
		seq_printf( s, "    VIDOUTF_FORMAT__SD720H: vdck_out_phase(0x%x)\n", 
				pr1000_reg_table_vidout_format[pHost->sysHost.stOutFormatAttr.muxChCnt][pHost->sysHost.stOutFormatAttr.b16bit][3].vdck_out_phase);
	}

	seq_printf( s, "---------------------- PR1000 chip info ---------------------\n");
	seq_printf( s, "Chip status.\n");
	for(i = 0; i < PR1000_CHIP_COUNT; i++)
	{
		seq_printf( s, "  [CHIP%d]:\n", i);
 		seq_printf( s, "    ChipID Verify(0x%04x): %d, RevID:0x%02X\n", PR1000_CHIPID, (pHost->sysHost.chipID_verify>>i)&1, pHost->sysHost.revID[i]);
	 	seq_printf( s, "    I2C RW Verify : %d (1: success, other: failed)\n", (pHost->sysHost.i2cRW_verify >> i)&0x1);
	}

	seq_printf( s, "---------------------- sys proc info -------------------------\n");
 	seq_printf( s, "Channel mapping: \n");
 	seq_printf( s, "    Inx Ch vdport prChip prChn i2cSlvAddr\n");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		seq_printf( s, "    %3d %2d %6d %6d %5d %10x\n", i, 
				pHost->sysHost.portChSel[i].chn,
				pHost->sysHost.portChSel[i].vdPort,
				pHost->sysHost.portChSel[i].prChip,
				pHost->sysHost.portChSel[i].prChn,
				pHost->sysHost.portChSel[i].i2cSlvAddr>>1);
	}
#ifndef DONT_SUPPORT_ETC_FUNC
 	seq_printf( s, "Core Power down attribute: \n");
 	seq_printf( s, "    prChip VDECCLK(0/1/2/3) AudioAlink VADC(0/1/2/3) AADC PLL(1/2)\n");
	for(i = 0; i < PR1000_CHIP_COUNT; i++)
	{
		stPrPwDown.prChip = i;
		if( (PR1000_GetPwDown(fd, &stPrPwDown)) < 0)
		{
			break;
		}
		seq_printf( s, "    %5d         %d/%d/%d/%d  %10d      %d/%d/%d/%d  %4d     %d/%d\n", 
				stPrPwDown.prChip,
				stPrPwDown.bVDEC_CLK_PD0, //prchn 0, page0 0xEC b'3
				stPrPwDown.bVDEC_CLK_PD1, //prchn 1, page0 0xEC b'7
				stPrPwDown.bVDEC_CLK_PD2, //prchn 2, page0 0xED b'3
				stPrPwDown.bVDEC_CLK_PD3, //prchn 3, page0 0xED b'7
				stPrPwDown.bAUDIO_ALINK_PD, //0xEA b'7
				stPrPwDown.bVADC_PD0, //prchn 0, page1 0x68 b'4
				stPrPwDown.bVADC_PD1, //prchn 1, page1 0xE8 b'4
				stPrPwDown.bVADC_PD2, //prchn 2, page2 0x68 b'4
				stPrPwDown.bVADC_PD3, //prchn 3, page2 0xE8 b'4
				stPrPwDown.AADC_PD, //page0 0xEB b'[7:5]
				stPrPwDown.bPLL1_PD, //page0 0xE6 b'7 All digital/anglog
				stPrPwDown.bPLL2_PD); //page0 0xE7 b'7 Only video output
	}
#endif // DONT_SUPPORT_ETC_FUNC
 	seq_printf( s, "User set attribute: \n");
 	seq_printf( s, "    Ch CameraType           Resolution\n");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

#ifndef DONT_SUPPORT_HELP_STRING 
		seq_printf( s, "    %2d %-20s %-50s\n", i, 
				_STR_PR1000_FORMAT[pHost->sysHost.gPR1000RxType[i]], 
				_STR_PR1000_OUTRESOL[pHost->sysHost.gPR1000RxResol[i]]);
#else // DONT_SUPPORT_HELP_STRING 
		seq_printf( s, "    %2d %-20d %-50d\n", i, 
				pHost->sysHost.gPR1000RxType[i], 
				pHost->sysHost.gPR1000RxResol[i]);
#endif // DONT_SUPPORT_HELP_STRING 
	}
#ifdef SUPPORT_HIDE_EQING_DISPLAY 
	seq_printf( s, "Cable EQ attribute(Hide EQing display): \n");
#else
	seq_printf( s, "Cable EQ attribute(Show EQing display): \n");
#endif
	seq_printf( s, "    Ch En Step Lock CTCnt ChgStd Man(Std/Ref/Res)\n");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		seq_printf( s, "    %2d %2d %4d %4d %5d %6d %d(%s)/%d(%s)/%d(%s)\n", i, 
				pHost->sysHost.stCEQDataList[i].bEnable, 
				pHost->sysHost.stCEQDataList[i].estStep, 
				pHost->sysHost.stCEQDataList[i].bLock, 
				pHost->sysHost.lastChromaLockTunn[i],
				pHost->sysHost.stCEQDataList[i].bForceChgStd, 
				pHost->sysHost.stCEQDataList[i].stManEQMan.b.man_ifmt_std, 
				(pHost->sysHost.stCEQDataList[i].stManEQMan.b.man_ifmt_res == PR1000_DET_IFMT_RES_480i) ? ("PR1000_DET_IFMT_STD_NTSC"):
					((pHost->sysHost.stCEQDataList[i].stManEQMan.b.man_ifmt_res == PR1000_DET_IFMT_RES_576i) ? ("PR1000_DET_IFMT_STD_PAL"):
					(_STR_PR1000_IFMT_STD[pHost->sysHost.stCEQDataList[i].stManEQMan.b.man_ifmt_std])),
				pHost->sysHost.stCEQDataList[i].stManEQMan.b.man_ifmt_ref, _STR_PR1000_IFMT_REF[pHost->sysHost.stCEQDataList[i].stManEQMan.b.man_ifmt_ref],
				pHost->sysHost.stCEQDataList[i].stManEQMan.b.man_ifmt_res, _STR_PR1000_IFMT_RES[pHost->sysHost.stCEQDataList[i].stManEQMan.b.man_ifmt_res]);
	}

	seq_printf( s, "    Ch  StepComplete status\n");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		seq_printf( s, "    %2d  ", i);
		for(j = PR1000_CEQ_STEP_NONE; j < MAX_PR1000_CEQ_STEP; j++)
		{
			if(j == (MAX_PR1000_CEQ_STEP-1) )
			{
				seq_printf( s, "%d", pHost->sysHost.stCEQDataList[i].flagStepComplete[j]);
			}
			else
			{
				if(j == PR1000_CEQ_STEP_EQSTDDONE) seq_printf( s, "%d(*)-", pHost->sysHost.stCEQDataList[i].flagStepComplete[j]);
				else seq_printf( s, "%d-", pHost->sysHost.stCEQDataList[i].flagStepComplete[j]);
			}
		}
		seq_printf( s, "\n");
	}
	seq_printf( s, "    Ch  MonitorEQ status\n");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		seq_printf( s, "    %2d  ", i);
		for(j = PR1000_MON_STEP_NONE; j < MAX_PR1000_MON_STEP; j++)
		{
			seq_printf( s, "%02x ", pHost->sysHost.stCEQDataList[i].flagMonStepComplete[j]);
		}
		seq_printf( s, "\n");
	}

	seq_printf( s, "    Ch  Comp1  Comp2 CompF Atten1 Atten2 AttenF\n");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		seq_printf( s, "    %2d 0x%04x 0x%04x %5d 0x%04x 0x%04x %6d\n", i, 
				(pHost->sysHost.stCEQDataList[i].infoRegs.b.det_eq_comp1_h<<8)|(pHost->sysHost.stCEQDataList[i].infoRegs.b.det_eq_comp1_l),
				(pHost->sysHost.stCEQDataList[i].infoRegs.b.det_eq_comp2_h<<8)|(pHost->sysHost.stCEQDataList[i].infoRegs.b.det_eq_comp2_l),
				pHost->sysHost.stCEQDataList[i].compFact,
				(pHost->sysHost.stCEQDataList[i].infoRegs.b.det_eq_atten1_h<<8)|(pHost->sysHost.stCEQDataList[i].infoRegs.b.det_eq_atten1_l),
				(pHost->sysHost.stCEQDataList[i].infoRegs.b.det_eq_atten2_h<<8)|(pHost->sysHost.stCEQDataList[i].infoRegs.b.det_eq_atten2_l),
				pHost->sysHost.stCEQDataList[i].attenFact);
	}


	seq_printf( s, "------------------ irq proc info -------------------------\n");
	seq_printf( s, "Irq Attribute: \n");
 	seq_printf( s, "    Chip IrqCtrl SyncPrd WakePrd SyncMode IRQState EventState\n");
	for(i = 0; i < PR1000_CHIP_COUNT; i++)
	{
		/* Read irq status. */
		if( PR1000_PageReadBurst(fd, PR1000_I2C_SLVADDRS[i], PR1000_REG_PAGE_COMMON, 0x80, sizeof(uint8_t)*7, (uint8_t *)u8Data) >= 0)
		{     
			seq_printf( s, "    %4d      %02x      %02x      %02x     %02x%02x       %02x         %02x\n", i,
				u8Data[0], u8Data[1], u8Data[2], u8Data[3],
				u8Data[4], u8Data[5], u8Data[6]);
		}     
	}
	seq_printf( s, "    Chip NovidMD GPIOMD[0/1/2/3/4/5]\n");
	for(i = 0; i < PR1000_CHIP_COUNT; i++)
	{
		/* Read irq status. */
		if( PR1000_PageReadBurst(fd, PR1000_I2C_SLVADDRS[i], PR1000_REG_PAGE_COMMON, 0x88, sizeof(uint8_t)*7, (uint8_t *)u8Data) >= 0)
		{     
			seq_printf( s, "    %4d      %02x %02x/%02x/%02x/%02x/%02x/%02x\n", i,
				u8Data[0], u8Data[1], u8Data[2], u8Data[3],
				u8Data[4], u8Data[5], u8Data[6]);
		}     
	}
	seq_printf( s, "    Chip NovidLV GPIOLV[0/1/2/3/4/5]\n");
	for(i = 0; i < PR1000_CHIP_COUNT; i++)
	{
		/* Read irq status. */
		if( PR1000_PageReadBurst(fd, PR1000_I2C_SLVADDRS[i], PR1000_REG_PAGE_COMMON, 0x90, sizeof(uint8_t)*7, (uint8_t *)u8Data) >= 0)
		{     
			seq_printf( s, "    %4d      %02x %02x/%02x/%02x/%02x/%02x/%02x\n", i,
				u8Data[0], u8Data[1], u8Data[2], u8Data[3],
				u8Data[4], u8Data[5], u8Data[6]);
		}     
	}
	seq_printf( s, "    Chip GPIOBoth[0/1/2/3/4/5]\n");
	for(i = 0; i < PR1000_CHIP_COUNT; i++)
	{
		/* Read irq status. */
		if( PR1000_PageReadBurst(fd, PR1000_I2C_SLVADDRS[i], PR1000_REG_PAGE_COMMON, 0x99, sizeof(uint8_t)*6, (uint8_t *)u8Data) >= 0)
		{     
			seq_printf( s, "    %4d %02x/%02x/%02x/%02x/%02x/%02x\n", i,
				u8Data[0], u8Data[1], u8Data[2], u8Data[3],
				u8Data[4], u8Data[5]);
		}     
	}
	
	seq_printf( s, "Irq Enable: \n");
 	seq_printf( s, "    Chip PTZ[0/1/2/3] VFD|NOVID BD|MD DD|ND ADMUTE|DFD ADDIFF|ADABS GPIO[0/1/2/3/4/5]\n");
	for(i = 0; i < PR1000_CHIP_COUNT; i++)
	{
		/* Read irq status. */
		if( PR1000_PageReadBurst(fd, PR1000_I2C_SLVADDRS[i], PR1000_REG_PAGE_COMMON, 0xA0, sizeof(stIRQStatus), (uint8_t *)&stIRQStatus) >= 0)
		{     
			seq_printf( s, "    %4d %02x/%02x/%02x/%02x         %02x    %02x    %02x         %02x           %02x %02x/%02x/%02x/%02x/%02x/%02x\n", i,
				stIRQStatus.u8PTZ[0], stIRQStatus.u8PTZ[1], stIRQStatus.u8PTZ[2], stIRQStatus.u8PTZ[3],
				stIRQStatus.u8NOVID,
				stIRQStatus.u8MD,
				stIRQStatus.u8ND,
				stIRQStatus.u8DFD,
				stIRQStatus.u8AD,
				stIRQStatus.u8GPIO0,
				stIRQStatus.u8GPIO1_5[0],
				stIRQStatus.u8GPIO1_5[1],
				stIRQStatus.u8GPIO1_5[2],
				stIRQStatus.u8GPIO1_5[3],
				stIRQStatus.u8GPIO1_5[4]);
		}     
	}
	
	seq_printf( s, "-------------------- vid proc info -------------------------\n");
 	seq_printf( s, "Vid Active/Delay attribute: \n");
 	seq_printf( s, "    Ch Hactive Hdelay Vactive Vdelay\n");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		if( PR1000_VID_GetChnAttr(fd, &pHost->sysHost.portChSel[i], &stChnAttr) >= 0)
		{
			seq_printf( s, "    %2d %7d %6d %7d %6d\n", i, 
					stChnAttr.u16HActive,
					stChnAttr.u16HDelay,
					stChnAttr.u16VActive,
					stChnAttr.u16VDelay);
		}
	}
#ifndef DONT_SUPPORT_VID_ENHANCEMENT
 	seq_printf( s, "CSC attribute: \n");
 	seq_printf( s, "    Ch CbGain CrGain CbOff CrOff \n");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		if(PR1000_VID_GetCscAttr(fd, &pHost->sysHost.portChSel[i], &stCscAttr) >= 0)
		{
			seq_printf( s, "    %2d %6x %6x %5x %5x\n", i, 
					stCscAttr.u8CbGain,
					stCscAttr.u8CrGain,
					stCscAttr.u8CbOffset,
					stCscAttr.u8CrOffset);
		}
	}
 	seq_printf( s, "Video setup: \n");
 	seq_printf( s, "    Ch Contrast Bright Saturation Hue Sharpness\n");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		seq_printf( s, "    %2d", i);
		if(PR1000_VID_GetContrast(fd, &pHost->sysHost.portChSel[i], &stContrast) >= 0)
		{
			seq_printf( s, " %8x", stContrast.u8Contrast);
		}
		if(PR1000_VID_GetBright(fd, &pHost->sysHost.portChSel[i], &stBright) >= 0)
		{
			seq_printf( s, " %6x", stBright.u8Bright);
		}
		if(PR1000_VID_GetSaturation(fd, &pHost->sysHost.portChSel[i], &stSaturation) >= 0)
		{
			seq_printf( s, " %10x", stSaturation.u8Saturation);
		}
		if(PR1000_VID_GetHue(fd, &pHost->sysHost.portChSel[i], &stHue) >= 0)
		{
			seq_printf( s, " %3x", stHue.u8Hue);
		}
		if(PR1000_VID_GetSharpness(fd, &pHost->sysHost.portChSel[i], &stSharpness) >= 0)
		{
			seq_printf( s, " %9x", stSharpness.u8Sharpness);
		}
		seq_printf( s, "\n");
	}
#endif // DONT_SUPPORT_VID_ENHANCEMENT

	seq_printf( s, "-------------------- event proc info -------------------------\n");
 	seq_printf( s, "NOVID attribute: \n");
 	seq_printf( s, "    Ch BlankColor(0:Black,1:Blue)\n");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		if(PR1000_VEVENT_GetNovidAttr(fd, &pHost->sysHost.portChSel[i], &stNovidAttr) >= 0)
		{
			seq_printf( s, "    %2d %10d\n", i, stNovidAttr.blankColor);
		}
	}
#ifndef DONT_SUPPORT_EVENT_FUNC
 	seq_printf( s, "MD attribute: \n");
 	seq_printf( s, "    Ch En MaskEn Lvsens Spsens Tmpsens Velocity\n");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		seq_printf( s, "    %2d", i); 
		if(PR1000_VEVENT_GetMdAttr(fd, &pHost->sysHost.portChSel[i], &stMdAttr) >= 0)
		{
			seq_printf( s, " %2d %6d", stMdAttr.bEn, stMdAttr.bMaskEn); 
		}
		if(PR1000_VEVENT_GetMdLvSens(fd, &pHost->sysHost.portChSel[i], &stMdLvSens) >= 0)
		{
			seq_printf( s, " %6x", stMdLvSens.lvsens);
		}
		if(PR1000_VEVENT_GetMdSpSens(fd, &pHost->sysHost.portChSel[i], &stMdSpSens) >= 0)
		{
			seq_printf( s, " %6x", stMdSpSens.spsens); 
		}
		if(PR1000_VEVENT_GetMdTmpSens(fd, &pHost->sysHost.portChSel[i], &stMdTmpSens) >= 0)
		{
			seq_printf( s, " %7x", stMdTmpSens.tmpsens); 
		}
		if(PR1000_VEVENT_GetMdVelocity(fd, &pHost->sysHost.portChSel[i], &stMdVelocity) >= 0)
		{
			seq_printf( s, " %5d", stMdVelocity.velocity); 
		}
		seq_printf( s, "\n");
	}
 	seq_printf( s, "BD attribute: \n");
 	seq_printf( s, "    Ch En MaskEn Lvsens Spsens Tmpsens\n");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		seq_printf( s, "    %2d", i); 
		if(PR1000_VEVENT_GetBdAttr(fd, &pHost->sysHost.portChSel[i], &stBdAttr) >= 0)
		{
			seq_printf( s, " %2d %6d", stBdAttr.bEn, stBdAttr.bMaskEn); 
		}
		if(PR1000_VEVENT_GetBdLvSens(fd, &pHost->sysHost.portChSel[i], &stBdLvSens) >= 0)
		{
			seq_printf( s, " %6x", stBdLvSens.lvsens); 
		}
		if(PR1000_VEVENT_GetBdSpSens(fd, &pHost->sysHost.portChSel[i], &stBdSpSens) >= 0)
		{
			seq_printf( s, " %6x", stBdSpSens.spsens); 
		}
		if(PR1000_VEVENT_GetBdTmpSens(fd, &pHost->sysHost.portChSel[i], &stBdTmpSens) >= 0)
		{
			seq_printf( s, " %7x", stBdTmpSens.tmpsens); 
		}
		seq_printf( s, "\n");
	}
 	seq_printf( s, "ND attribute: \n");
 	seq_printf( s, "    Ch En Lvsens_low Lvsens_high Tmpsens\n");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		seq_printf( s, "    %2d", i); 
		if(PR1000_VEVENT_GetNdAttr(fd, &pHost->sysHost.portChSel[i], &stNdAttr) >= 0)
		{
			seq_printf( s, " %2d", stNdAttr.bEn); 
		}
		if(PR1000_VEVENT_GetNdLvSens(fd, &pHost->sysHost.portChSel[i], &stNdLvSens) >= 0)
		{
			seq_printf( s, " %10x %11x", stNdLvSens.lvsens_low, stNdLvSens.lvsens_high); 
		}
		if(PR1000_VEVENT_GetNdTmpSens(fd, &pHost->sysHost.portChSel[i], &stNdTmpSens) >= 0)
		{
			seq_printf( s, " %7x", stNdTmpSens.tmpsens); 
		}
		seq_printf( s, "\n");
	}
 	seq_printf( s, "DD attribute: \n");
	seq_printf( s, "    Ch En Lvsens_low Lvsens_high Tmpsens\n");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		seq_printf( s, "    %2d", i); 
		if(PR1000_VEVENT_GetDdAttr(fd, &pHost->sysHost.portChSel[i], &stDdAttr) >= 0)
		{
			seq_printf( s, " %2d", stDdAttr.bEn); 
		}
		if(PR1000_VEVENT_GetDdLvSens(fd, &pHost->sysHost.portChSel[i], &stDdLvSens) >= 0)
		{
			seq_printf( s, " %10x %11x", stDdLvSens.lvsens_low, stDdLvSens.lvsens_high); 
		}
		if(PR1000_VEVENT_GetDdTmpSens(fd, &pHost->sysHost.portChSel[i], &stDdTmpSens) >= 0)
		{
			seq_printf( s, " %7x", stDdTmpSens.tmpsens); 
		}
		seq_printf( s, "\n");
	}
 	seq_printf( s, "DFD attribute: \n");
 	seq_printf( s, "    Ch Vstart Hstart Vsize Hsize Lvsens Spsens Tmpsens\n");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		seq_printf( s, "    %2d", i); 
		if(PR1000_VEVENT_GetDfdAttr(fd, &pHost->sysHost.portChSel[i], &stDfdAttr) >= 0)
		{
			seq_printf( s, " %6x %6x %5x %5x", 
					stDfdAttr.vstart, 
					stDfdAttr.hstart, 
					stDfdAttr.vsize, 
					stDfdAttr.hsize); 
		}
		if(PR1000_VEVENT_GetDfdLvSens(fd, &pHost->sysHost.portChSel[i], &stDfdLvSens) >= 0)
		{
			seq_printf( s, " %6x", stDfdLvSens.lvsens); 
		}
		if(PR1000_VEVENT_GetDfdSpSens(fd, &pHost->sysHost.portChSel[i], &stDfdSpSens) >= 0)
		{
			seq_printf( s, " %6x", stDfdSpSens.spsens); 
		}
		if(PR1000_VEVENT_GetDfdTmpSens(fd, &pHost->sysHost.portChSel[i], &stDfdTmpSens) >= 0)
		{
			seq_printf( s, " %7x", stDfdTmpSens.tmpsens); 
		}
		seq_printf( s, "\n");

	}
 	seq_printf( s, "Mask cell attribute: \n");
 	seq_printf( s, "    Ch CntX CntY Width Height VStrtOs HStrtOs resol\n");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		memset(&stMaskCellAttr, 0, sizeof(_stMaskCellAttr));
		PR1000_VEVENT_GetMaskAttr(fd, &pHost->sysHost.portChSel[i], pHost->sysHost.gPR1000RxResol[i], &stMaskCellAttr);
		seq_printf( s, "    %2d %4d %4d %5d %6d %7d %7d %-s(%s)\n", i, 
				stMaskCellAttr.cellCntX,
				stMaskCellAttr.cellCntY,
				stMaskCellAttr.cellWidth,
				stMaskCellAttr.cellHeight,
				stMaskCellAttr.cellVStartOffset,
				stMaskCellAttr.cellHStartOffset,
				_STR_PR1000_OUTRESOL[pHost->sysHost.gPR1000RxResol[i]],
				"If CVBS(i), (CntYxHeight)=half resol");
	}
 	seq_printf( s, "Display mask cell attribute: \n");
 	seq_printf( s, "    Ch Type EnMaskPln EnDetPln maskBlendLv detBndrLv detBndrWid maskBndrLv maskBndrWid maskColor\n");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		seq_printf( s, "    %2d %d:%2s %9d %8d %11x %9x %10x %10x %11x %d:%-7s\n", i, 
				pHost->eventHost.stVEVENTDisplayAttr[i].cellFormat, 
				(pHost->eventHost.stVEVENTDisplayAttr[i].cellFormat==0)?"PZ":((pHost->eventHost.stVEVENTDisplayAttr[i].cellFormat==1)?"BD":"MD"),
				pHost->eventHost.stVEVENTDisplayAttr[i].bEnMaskPln, 
				pHost->eventHost.stVEVENTDisplayAttr[i].bEnDetPln, 
				pHost->eventHost.stVEVENTDisplayAttr[i].maskBlendLevel, 
				pHost->eventHost.stVEVENTDisplayAttr[i].detBndrLevel, 
				pHost->eventHost.stVEVENTDisplayAttr[i].detBndrWidth, 
				pHost->eventHost.stVEVENTDisplayAttr[i].maskBndrLevel, 
				pHost->eventHost.stVEVENTDisplayAttr[i].maskBndrWidth, 
				pHost->eventHost.stVEVENTDisplayAttr[i].maskColor,
				_STR_PR1000_MASKCOLOR[pHost->eventHost.stVEVENTDisplayAttr[i].maskColor]); 
	}
#endif // DONT_SUPPORT_EVENT_FUNC
 	seq_printf( s, "DetCamera Set: \n");
 	seq_printf( s, "    Ch DetCameraType        DetCameraResol\n");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

#ifndef DONT_SUPPORT_HELP_STRING 
		seq_printf( s, "    %2d %-20s %-50s\n", i, 
				_STR_PR1000_FORMAT[pHost->eventHost.stEventDetStd[i].format], 
				_STR_PR1000_INRESOL[pHost->eventHost.stEventDetStd[i].resol]);
#else // DONT_SUPPORT_HELP_STRING 
		seq_printf( s, "    %2d %-20d %-50d\n", i, 
				pHost->eventHost.stEventDetStd[i].format, 
				pHost->eventHost.stEventDetStd[i].resol);
#endif // DONT_SUPPORT_HELP_STRING 
	}
#ifndef DONT_SUPPORT_PTZ_FUNC
	seq_printf( s, "-------------------- ptz proc info -------------------------\n");
 	seq_printf( s, "Ptz attribute: \n");
 	seq_printf( s, "    Ch Init CameraType           CameraResol\n");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		seq_printf( s, "    %2d %4d %-20s %-50s\n", i, 
				pHost->ptzHost.initialized[i], 
				_STR_PR1000_FORMAT[pHost->ptzHost.ptzType[i]], 
				_STR_PR1000_OUTRESOL[pHost->ptzHost.ptzResol[i]]);
	}
 	seq_printf( s, "Ptz Tx attribute: \n");
 	seq_printf( s, "    Ch fType fPol start pEn hstOs lCnt hst dPol freqFirst freqency   hpst lLen aDLen lRNum rptEn cGNum tpSel gEn\n");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		if(PR1000_PTZ_GetTxAttr(fd, &pHost->sysHost.portChSel[i], &stPTZTxAttr) >= 0)
		{
			seq_printf( s, "    %2d %5d %4d %5d %3d %5d %4d %3d %4d 0x%07x 0x%06x 0x%04x %4d %5d %5d %5d %5d %5d %3d\n", i, 
					stPTZTxAttr.b.fieldType,
					stPTZTxAttr.b.fieldPol,
					stPTZTxAttr.b.start,
					stPTZTxAttr.b.pathEn,
					stPTZTxAttr.b.hstOs,
					stPTZTxAttr.b.lineCnt,
					stPTZTxAttr.b.hst,
					stPTZTxAttr.b.dataPol,
					(stPTZTxAttr.b.freqFirst23<<16)| (stPTZTxAttr.b.freqFirst15<<8) | stPTZTxAttr.b.freqFirst07,
					(stPTZTxAttr.b.freq23<<16) | (stPTZTxAttr.b.freq15<<8) | stPTZTxAttr.b.freq07,
					(stPTZTxAttr.b.hpst12<<8) | stPTZTxAttr.b.hpst07,
					stPTZTxAttr.b.lineLen,
					stPTZTxAttr.b.allDataLen,
					stPTZTxAttr.b.lastRptNum,
					stPTZTxAttr.b.rptEn,
					stPTZTxAttr.b.cmdGrpNum,
					stPTZTxAttr.b.tpSel,
					stPTZTxAttr.b.grpEn);
		}
	}
 	seq_printf( s, "Ptz Rx attribute: \n");
 	seq_printf( s, "    Ch fType fPol iFrmEn iLEn start pEn hstOs lCnt hst dPol freqFirst freqency lpfLen pOff lLen vCnt tpSel aHEn tEn\n");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		if(PR1000_PTZ_GetRxAttr(fd, &pHost->sysHost.portChSel[i], &stPTZRxAttr) >= 0)
		{
			seq_printf( s, "    %2d %5d %4d %6d %4d %5d %3d %5d %4d %3d %4d 0x%07x 0x%06x %6d %4d %4d %4d %5d %4d %3d\n", i, 
					stPTZRxAttr.b.fieldType,
					stPTZRxAttr.b.fieldPol,
					stPTZRxAttr.b.ignoreFrmEn,
					stPTZRxAttr.b.ignoreLineEn,
					stPTZRxAttr.b.start,
					stPTZRxAttr.b.pathEn,
					stPTZRxAttr.b.hstOs,
					stPTZRxAttr.b.lineCnt,
					stPTZRxAttr.b.hst,
					stPTZRxAttr.b.dataPol,
					(stPTZRxAttr.b.freqFirst23<<16) | (stPTZRxAttr.b.freqFirst15<<8) | stPTZRxAttr.b.freqFirst07,
					(stPTZRxAttr.b.freq23<<16) | (stPTZRxAttr.b.freq15<<8) | stPTZRxAttr.b.freq07,
					stPTZRxAttr.b.lpfLen,
					stPTZRxAttr.b.pixOffset,
					stPTZRxAttr.b.lineLen,
					stPTZRxAttr.b.validCnt,
					stPTZRxAttr.b.tpSel,
					stPTZRxAttr.b.addrHoldEn,
					stPTZRxAttr.b.testEn);
		}
	}
 	seq_printf( s, "Ptz HV Start attribute: \n");
 	seq_printf( s, "    Ch rxHstartOs rxHSyncPol rxVstartOs rxVsyncPol txHstartOs txHSyncPol txVstartOs txVsyncPol\n");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		if(PR1000_PTZ_GetHVStartAttr(fd, &pHost->sysHost.portChSel[i], &stPTZHVStartAttr) >= 0)
		{
			seq_printf( s, "    %2d %10d %10d %10d %10d %10d %10d %10d %10d\n", i, 
					(stPTZHVStartAttr.b.rxHstrtOs13<<8) | stPTZHVStartAttr.b.rxHstrtOs07,
					stPTZHVStartAttr.b.rxHsyncPol,
					(stPTZHVStartAttr.b.rxVstrtOs10<<8) | stPTZHVStartAttr.b.rxVstrtOs07,
					stPTZHVStartAttr.b.rxVsyncPol,
					(stPTZHVStartAttr.b.txHstrtOs13<<8) | stPTZHVStartAttr.b.txHstrtOs07,
					stPTZHVStartAttr.b.txHsyncPol,
					(stPTZHVStartAttr.b.txVstrtOs10<<8) | stPTZHVStartAttr.b.txVstrtOs07,
					stPTZHVStartAttr.b.txVsyncPol);

		}
	}
 	seq_printf( s, "Current Tx/Rx status: \n");
 	seq_printf( s, "    Ch TxCnt    RxCnt(fifo)    IsrBitStatus\n");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		seq_printf( s, "    %2d %08x %08x(%04d) %08lx\n", i, 
				pHost->ptzHost.u32PtzTxCnt[i], 
				pHost->ptzHost.u32PtzRxCnt[i],
				PR1000_CQ_howmany(&pHost->ptzHost.ptzRecvQ[i], &pHost->ptzHost.splockPtzRecvQ[i]),
				pHost->ptzHost.bitPtzIsrStatus[i]); 
	}
#endif // DONT_SUPPORT_PTZ_FUNC

#ifndef DONT_SUPPORT_AUD_ALINK
	seq_printf( s, "------------------ audio proc info -------------------------\n");
 	seq_printf( s, "Aud Cascade attribute: \n");
 	seq_printf( s, "    Chip cascade\n");
	for(i = 0; i < PR1000_CHIP_COUNT; i++)
	{
		if(PR1000_AUD_GetCascadeAttr(fd, i, &stAUDCascadeAttr) >= 0)
		{
			seq_printf( s, "    %4d %7s\n", i, 
					(stAUDCascadeAttr.bCascadeMaster==1)?"Master":"Slave");
		}
	}

 	seq_printf( s, "Aud Ai gain: \n");
 	seq_printf( s, "    Chip ch0/ch1/ch2/ch3\n");
	for(i = 0; i < PR1000_CHIP_COUNT; i++)
	{
		if( PR1000_PageReadBurst(fd, PR1000_I2C_SLVADDRS[i], PR1000_REG_PAGE_AUD, 0x02, sizeof(uint8_t)*3, (uint8_t *)u8Data) >= 0)
		{
			seq_printf( s, "    %4d  %1d / %1d / %1d / %1d \n", i, 
					(u8Data[0]>>4)&0xF,
					u8Data[1]&0xF,
					(u8Data[1]>>4)&0xF,
					u8Data[2]&0xF);
		}
	}
 	seq_printf( s, "Aud Mute: \n");
 	seq_printf( s, "    Chip Voice Mix Dac Rec[15..0]\n");
	for(i = 0; i < PR1000_CHIP_COUNT; i++)
	{
		if( PR1000_PageReadBurst(fd, PR1000_I2C_SLVADDRS[i], PR1000_REG_PAGE_AUD, 0x0A, sizeof(uint8_t)*6, (uint8_t *)u8Data) >= 0)
		{
			seq_printf( s, "    %4d %5d %3d %3d %04x\n", i, 
					(u8Data[1]>>7)&0x1,
					(u8Data[1]>>6)&0x1,
					(u8Data[0]>>4)&0x1,
					(u8Data[4]<<8)|u8Data[5]);
		}
	}

 	seq_printf( s, "Aud Dac: \n");
 	seq_printf( s, "    Chip Gain SelCh(16:pb,17:voice,18:mix)\n");
	for(i = 0; i < PR1000_CHIP_COUNT; i++)
	{
		if( PR1000_PageReadBurst(fd, PR1000_I2C_SLVADDRS[i], PR1000_REG_PAGE_AUD, 0x0A, sizeof(uint8_t)*2, (uint8_t *)u8Data) >= 0)
		{
			seq_printf( s, "    %4d %4d %5d\n", i, 
					u8Data[0]&0xF,
					u8Data[1]&0x1F);
		}
	}

 	seq_printf( s, "Aud I2S IOB(0:output,1:input): \n");
 	seq_printf( s, "    Chip Alink Pb(DAT|SYNC|CLK) Rec(DAT|SYNC|CLK)\n");
	for(i = 0; i < PR1000_CHIP_COUNT; i++)
	{
		if( (PR1000_PageRead(fd, PR1000_I2C_SLVADDRS[i], PR1000_REG_PAGE_COMMON, 0xD5, &u8Data[0])) < 0)
		{
			continue;
		}
		seq_printf( s, "    %4d %5d %6d|%4d|%3d  %7d|%4d|%3d  \n", i, 
			(u8Data[0]>>6)&0x1,
			(u8Data[0]>>5)&0x1,
			(u8Data[0]>>4)&0x1,
			(u8Data[0]>>3)&0x1,
			(u8Data[0]>>2)&0x1,
			(u8Data[0]>>1)&0x1,
			(u8Data[0]>>0)&0x1);
	}
	
 	seq_printf( s, "Aud Rec attribute: \n");
 	seq_printf( s, "    Chip Master DSP ClkRis Format     SplRate 8bit BitRate\n");
	for(i = 0; i < PR1000_CHIP_COUNT; i++)
	{
		if(PR1000_AUD_GetRecAttr(fd, i, &stAUDRecAttr) >= 0)
		{
			seq_printf( s, "    %4d %6d %3d %6d %d:%-6s %d:%-5s %4d %d:%-5s\n", i, 
					stAUDRecAttr.bMaster, 
					stAUDRecAttr.bDSP, 
					stAUDRecAttr.bClkRise, 
					stAUDRecAttr.format, 
					_STR_PR1000_AUD_REC_FORMAT[stAUDRecAttr.format], 
					stAUDRecAttr.sampleRate, 
					_STR_PR1000_AUD_SAMRATE[stAUDRecAttr.sampleRate], 
					stAUDRecAttr.b8bits, 
					stAUDRecAttr.bitRate, 
					_STR_PR1000_AUD_BITRATE[stAUDRecAttr.bitRate]); 
		}

	}
 	seq_printf( s, "Aud Pb attribute: \n");
 	seq_printf( s, "    Chip Master DSP RightCh Format      SplRate 8bit 8bitSelLow BitRate\n");
	for(i = 0; i < PR1000_CHIP_COUNT; i++)
	{
		if(PR1000_AUD_GetPbAttr(fd, i, &stAUDPbAttr) >= 0)
		{
			seq_printf( s, "    %4d %6d %3d %7d %d:%-9s %d:%-5s %4d %10d %d:%-5s\n", i,
					stAUDPbAttr.bMaster, 
					stAUDPbAttr.bDSP, 
					stAUDPbAttr.bRightChn, 
					stAUDPbAttr.format,
					_STR_PR1000_AUD_PB_FORMAT[stAUDPbAttr.format], 
					stAUDPbAttr.sampleRate, 
					_STR_PR1000_AUD_SAMRATE[stAUDPbAttr.sampleRate], 
					stAUDPbAttr.b8bits, 
					stAUDPbAttr.b8bitLow, 
					stAUDPbAttr.bitRate, 
					_STR_PR1000_AUD_BITRATE[stAUDPbAttr.bitRate]); 
		}
	}
#endif // DONT_SUPPORT_AUD_ALINK

	seq_printf( s, "\n");
	seq_printf( s, "--------------------------------------------------------------\n");
	seq_printf( s, "Count ISR Cnt:0x%x\n", pHost->sysHost.u32RxIntCnt);
	seq_printf( s, "ISR interval time:%dmsec\n", pHost->sysHost.u32IntIntervalTimeMsec);
	seq_printf( s, "------------------ current irq status -------------------------\n");
 	seq_printf( s, "Chip PTZ[0/1/2/3] VFD|NOVID BD|MD DD|ND ADMUTE|DFD ADDIFF|ADABS GPIO[0/1/2/3/4/5]\n");
	for(i = 0; i < PR1000_CHIP_COUNT; i++)
	{
		/* Read irq status. */
		if( PR1000_PageReadBurst(fd, PR1000_I2C_SLVADDRS[i], PR1000_REG_PAGE_COMMON, 0xC0, sizeof(stIRQStatus), (uint8_t *)&stIRQStatus) >= 0)
		{     
			seq_printf( s, "%4d %02x/%02x/%02x/%02x         %02x    %02x    %02x         %02x           %02x %02x/%02x/%02x/%02x/%02x/%02x\n", i,
				stIRQStatus.u8PTZ[0], stIRQStatus.u8PTZ[1], stIRQStatus.u8PTZ[2], stIRQStatus.u8PTZ[3],
				stIRQStatus.u8NOVID,
				stIRQStatus.u8MD,
				stIRQStatus.u8ND,
				stIRQStatus.u8DFD,
				stIRQStatus.u8AD,
				stIRQStatus.u8GPIO0,
				stIRQStatus.u8GPIO1_5[0],
				stIRQStatus.u8GPIO1_5[1],
				stIRQStatus.u8GPIO1_5[2],
				stIRQStatus.u8GPIO1_5[3],
				stIRQStatus.u8GPIO1_5[4]);
		}     
	}

	seq_printf( s, "-------------------- current detect format -------------------------\n");
 	seq_printf( s, "prchip/prchn Status\n");
	for(i = 0; i < PR1000_CHIP_COUNT; i++)
	{
		/* Read irq status. */
		for(ch = 0; ch < DEF_PR1000_MAX_CHN; ch++)
		{
			i2cReg = 0x00 + PR1000_OFFSETADDR_CEQ_INFO_CH(ch);
			if( PR1000_PageReadBurst(fd, PR1000_I2C_SLVADDRS[i], PR1000_REG_PAGE_COMMON, i2cReg, 0x14, reg) >= 0)
			{     
				seq_printf( s, "%d/%d Lock[0x00..0x03] [%02x,%02x,%02x,%02x]\n", i, ch,
					reg[0], reg[1], reg[2], reg[3]);
				if( (reg[0]|reg[1]|reg[2]|reg[3]) != 0)
				{
					stCeqDet.reg = reg[0];
					stManEQMan.reg = reg[0x10];
					seq_printf( s, "   Detect:%d(%s)/%d(%s)/%d(%s)\n",
							stCeqDet.b.det_ifmt_std, 
							(stCeqDet.b.det_ifmt_res == PR1000_DET_IFMT_RES_480i) ? ("PR1000_IFMT_STD_NTSC"):
							((stCeqDet.b.det_ifmt_res == PR1000_DET_IFMT_RES_576i) ? ("PR1000_IFMT_STD_PAL"):
							 (_STR_PR1000_IFMT_STD[stCeqDet.b.det_ifmt_std])),
							stCeqDet.b.det_ifmt_ref, _STR_PR1000_IFMT_REF[stCeqDet.b.det_ifmt_ref],
							stCeqDet.b.det_ifmt_res, _STR_PR1000_IFMT_RES[stCeqDet.b.det_ifmt_res]);
					seq_printf( s, "   CurSet:%d(%s)/%d(%s)/%d(%s)\n",
							stManEQMan.b.man_ifmt_std, 
							(stManEQMan.b.man_ifmt_res == PR1000_DET_IFMT_RES_480i) ? ("PR1000_IFMT_STD_NTSC"):
							((stManEQMan.b.man_ifmt_res == PR1000_DET_IFMT_RES_576i) ? ("PR1000_IFMT_STD_PAL"):
							 (_STR_PR1000_IFMT_STD[stManEQMan.b.man_ifmt_std])),
							stManEQMan.b.man_ifmt_ref, _STR_PR1000_IFMT_REF[stManEQMan.b.man_ifmt_ref],
							stManEQMan.b.man_ifmt_res, _STR_PR1000_IFMT_RES[stManEQMan.b.man_ifmt_res]);
					seq_printf( s, "   EQ_Gain:DC(%04x),AC(%04x)\n", reg[4]<<8|reg[5], reg[6]<<8|reg[7]);
					seq_printf( s, "   EQ_Comp:C1(%04x),C2(%04x)\n", reg[8]<<8|reg[9], reg[0xA]<<8|reg[0xB]);
					seq_printf( s, "   EQ_Atten:A1(%04x),A2(%04x)\n", reg[0xC]<<8|reg[0xD], reg[0xE]<<8|reg[0xF]);
					seq_printf( s, "   MAN_IF:IFMT(%04x),MAN_EQ(%04x)\n", reg[0x10]<<8|reg[0x11], reg[0x12]<<8|reg[0x13]);
				}
			}     
		}     
	}

	seq_printf( s, "-------------------- current event status -------------------------\n");
 	seq_printf( s, "Novid status(mapChn0..): \n    ");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		PR1000_VEVENT_GetNovidStatus(fd, &pHost->sysHost.portChSel[i], &status);
		seq_printf( s, "%d ", status&1);
	}
	seq_printf( s, "\n");

 	seq_printf( s, "Vfd status(mapChn0..): \n    ");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		PR1000_VEVENT_GetVfdStatus(fd, &pHost->sysHost.portChSel[i], &status);
		seq_printf( s, "%d ", status&1);
	}
	seq_printf( s, "\n");

#ifndef DONT_SUPPORT_EVENT_FUNC
 	seq_printf( s, "Md status(mapChn0..): \n    ");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		PR1000_VEVENT_GetMdStatus(fd, &pHost->sysHost.portChSel[i], &status);
		seq_printf( s, "%d ", status&1);
	}
	seq_printf( s, "\n");

 	seq_printf( s, "Bd status(mapChn0..): \n    ");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		PR1000_VEVENT_GetBdStatus(fd, &pHost->sysHost.portChSel[i], &status);
		seq_printf( s, "%d ", status&1);
	}
	seq_printf( s, "\n");

 	seq_printf( s, "Nd status(mapChn0..): \n    ");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		PR1000_VEVENT_GetNdStatus(fd, &pHost->sysHost.portChSel[i], &status);
		seq_printf( s, "%d ", status&1);
	}
	seq_printf( s, "\n");

 	seq_printf( s, "Dd status(mapChn0..): \n    ");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		PR1000_VEVENT_GetDdStatus(fd, &pHost->sysHost.portChSel[i], &status);
		seq_printf( s, "%d ", status&1);
	}
	seq_printf( s, "\n");

 	seq_printf( s, "Dfd status(mapChn0..): \n    ");
	for(i = 0; i < MAX_CHN; i++)
	{
		if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[i].i2cSlvAddr)) continue;

		PR1000_VEVENT_GetDfdStatus(fd, &pHost->sysHost.portChSel[i], &status);
		seq_printf( s, "%d ", status&1);
	}
	seq_printf( s, "\n");

	seq_printf( s, "AdMute status(mapChn0..): \n    ");
	for(i = 0; i < PR1000_CHIP_COUNT; i++)
	{
		for(ch = 0; ch < DEF_PR1000_MAX_CHN; ch++)
		{
			PR1000_VEVENT_GetAdMuteStatus(fd, i, ch, &status);
			seq_printf( s, "%d ", status&1);
		}
	}
	seq_printf( s, "\n");

	seq_printf( s, "AdAbs status(mapChn0..): \n    ");
	for(i = 0; i < PR1000_CHIP_COUNT; i++)
	{
		for(ch = 0; ch < DEF_PR1000_MAX_CHN; ch++)
		{
			PR1000_VEVENT_GetAdAbsStatus(fd, i, ch, &status);
			seq_printf( s, "%d ", status&1);
		}
	}
	seq_printf( s, "\n");

	seq_printf( s, "AdDiff status(mapChn0..): \n    ");
	for(i = 0; i < PR1000_CHIP_COUNT; i++)
	{
		for(ch = 0; ch < DEF_PR1000_MAX_CHN; ch++)
		{
			PR1000_VEVENT_GetAdDiffStatus(fd, i, ch, &status);
			seq_printf( s, "%d ", status&1);
		}
	}
	seq_printf( s, "\n");
#endif // DONT_SUPPORT_EVENT_FUNC

	seq_printf( s, "--------------------------------------------------------------\n");


	return 0;
}

static int Pr1000ProcOpen(struct inode *inode, struct file *file)
{
	int err;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
	_drvHost *pHost = PDE_DATA(inode);
#else
	_drvHost *pHost = PDE(inode)->data;
#endif //LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)

	if(!try_module_get(THIS_MODULE))
		return -ENODEV;

	err = single_open(file, Pr1000SeqShow, pHost);
	if(err)
		module_put(THIS_MODULE);
	return err;
};

static int Pr1000ProcRelease(struct inode *inode, struct file *file)
{
        int res = single_release(inode, file);
        module_put(THIS_MODULE);
        return res;
}

static const struct file_operations pr1000ProcFileOps = {
	.owner   = THIS_MODULE,
	.open    = Pr1000ProcOpen,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = Pr1000ProcRelease,
};

#endif // SUPPORT_PROC_SYSTEM
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef CONFIG_HISI_SNAPSHOT_BOOT
static int pr1000_freeze(struct himedia_device* pdev)
{
    printk(KERN_ALERT "%s  %d\n", __FUNCTION__, __LINE__);
    return 0;
}

static int pr1000_restore(struct himedia_device* pdev)
{  
    printk(KERN_ALERT "%s  %d\n", __FUNCTION__, __LINE__);
    return 0;
}
#endif // CONFIG_HISI_SNAPSHOT_BOOT

//////////////////////////////////////////////////////////////////////////

#ifdef __LINUX_SYSTEM__
int pr1000_open(struct inode * inode, struct file * file)
{
	DbgString("opened\n");
	return 0;
} 
#else //#ifdef __LINUX_SYSTEM__
int pr1000_open(void)
{
	DbgString("opened\n");
	return 0;
} 
#endif // __LINUX_SYSTEM__

#ifdef __LINUX_SYSTEM__
int pr1000_close(struct inode * inode, struct file * file)
{
	DbgString("closed\n");
	return 0;
}
#else //#ifdef __LINUX_SYSTEM__
int pr1000_close(void)
{
	DbgString("closed\n");
	return 0;
}
#endif // __LINUX_SYSTEM__

#ifdef __LINUX_SYSTEM__
unsigned int pr1000_poll(struct file *pFile, poll_table *pWait)
{
        _drvHost *pHost = (_drvHost *)gpDrvHost;

        unsigned int mask = 0; 

        poll_wait( pFile, &pHost->wqPoll, pWait);

        if( (pHost->wqPollChnStatus.bitWqPollDetStd != 0) || (pHost->wqPollChnStatus.bitWqPollNovid != 0) ||
        	(pHost->wqPollChnStatus.bitWqPollVfd != 0) || (pHost->wqPollChnStatus.bitWqPollMd != 0) ||
        	(pHost->wqPollChnStatus.bitWqPollBd != 0) || (pHost->wqPollChnStatus.bitWqPollNd != 0) ||
        	(pHost->wqPollChnStatus.bitWqPollDd != 0) || (pHost->wqPollChnStatus.bitWqPollDfd != 0) ||
        	(pHost->wqPollChnStatus.bitWqPollAdMute != 0) || (pHost->wqPollChnStatus.bitWqPollAdAbs != 0) ||
        	(pHost->wqPollChnStatus.bitWqPollAdDiff != 0) || (pHost->wqPollChnStatus.bitWqPollGpio != 0) ||
        	(pHost->wqPollChnStatus.bitWqPollPtzRx != 0) )
        {    
                mask = POLLIN | POLLRDNORM;
        }    

        return mask;
}
#else //#ifdef __LINUX_SYSTEM__
unsigned int pr1000_poll(unsigned int waitMsec)
{
        _drvHost *pHost = (_drvHost *)gpDrvHost;

        unsigned int mask = 0; 

	do
	{
		if(pHost->wqPoll) break;

		MDelay(10);
		if(waitMsec >= 10) 
		{
			waitMsec-=10;
		}
		else 
		{
			MDelay(waitMsec);
			if(pHost->wqPoll) break;
			waitMsec = 0;
		}

	} while(waitMsec>0);

	if(waitMsec == 0) return(0);

	if( (pHost->wqPollChnStatus.bitWqPollDetStd != 0) || (pHost->wqPollChnStatus.bitWqPollNovid != 0) ||
        	(pHost->wqPollChnStatus.bitWqPollVfd != 0) || (pHost->wqPollChnStatus.bitWqPollMd != 0) ||
        	(pHost->wqPollChnStatus.bitWqPollBd != 0) || (pHost->wqPollChnStatus.bitWqPollNd != 0) ||
        	(pHost->wqPollChnStatus.bitWqPollDd != 0) || (pHost->wqPollChnStatus.bitWqPollDfd != 0) ||
        	(pHost->wqPollChnStatus.bitWqPollAdMute != 0) || (pHost->wqPollChnStatus.bitWqPollAdAbs != 0) ||
        	(pHost->wqPollChnStatus.bitWqPollAdDiff != 0) || (pHost->wqPollChnStatus.bitWqPollGpio != 0) ||
        	(pHost->wqPollChnStatus.bitWqPollPtzRx != 0) )
        {    
                mask = POLLIN | POLLRDNORM;
        }    

        return mask;
}
#endif // __LINUX_SYSTEM__

#ifdef __LINUX_SYSTEM__
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long pr1000_ioctl(struct file *pFile, unsigned int cmd, unsigned long arg)  
#else
int pr1000_ioctl(struct inode *pInode, struct file *pFile, unsigned int cmd, unsigned long arg)  
#endif
{
        _drvHost *pHost = (_drvHost *)gpDrvHost;
	void __user *pArg = (void __user *)arg;
	int ret = 0;
	const int fd = 0; //Normal:0, Isr:-1

        if( (_IOC_TYPE(cmd) != PR_IOC_MAGIC) ) 
                return -ENOTTY;

        switch(cmd) 
        {    
		case PR_IOC_RESET:
		{
		}
		break;
		case PR_IOS_REGWRITE:
		{/*{{{*/
			_stPrReg stPrReg;

			DbgString("PR_IOC_REGWRITE\n");

			if( (ret = copy_from_user((void *)&stPrReg, pArg, sizeof(_stPrReg))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			if( (ret = PR1000_PageWrite(fd, stPrReg.slvAddr, stPrReg.page, stPrReg.reg, stPrReg.data)) < 0)
			{
				DbgString("Write reg.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_REGREAD:
		{/*{{{*/
			_stPrReg stPrReg;

			DbgString("PR_IOG_REGREAD\n");

			if( (ret = copy_from_user((void *)&stPrReg, pArg, sizeof(_stPrReg))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			if( (ret = PR1000_PageRead(fd, stPrReg.slvAddr, stPrReg.page, stPrReg.reg, &stPrReg.data)) < 0)
			{
				DbgString("Read reg.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stPrReg, sizeof(_stPrReg))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_RXMODE:
		{/*{{{*/
			_stPrRxMode stRxmode;
			int mapChn;
			uint8_t type;
			uint8_t resol;
			_stCeqLock stCeqLock;

			_stCEQData *pstCEQData = NULL;
			_stPortChSel *pstPortChSel = NULL;
			uint8_t prChip, prChn, i2cSlaveAddr; 
			uint8_t i2cReg = 0;

			if( (ret = copy_from_user((void *)&stRxmode, pArg, sizeof(_stPrRxMode))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_RXMODE(mapChn:%d)\n", stRxmode.mapChn);

			if( (stRxmode.mapChn >= MAX_CHN) || (stRxmode.type >= max_pr1000_format) || (stRxmode.resol >= max_pr1000_outresol) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("set rxmode mapChn:%d, type:%d(%s), resol:%d(%s)\n", 
					stRxmode.mapChn COMMA 
					stRxmode.type COMMA _STR_PR1000_FORMAT[stRxmode.type] COMMA
					stRxmode.resol COMMA _STR_PR1000_OUTRESOL[stRxmode.resol]);

			pHost->sysHost.gPR1000RxType[stRxmode.mapChn] = stRxmode.type;
			pHost->sysHost.gPR1000RxResol[stRxmode.mapChn] = stRxmode.resol;

			mapChn = stRxmode.mapChn;
			type = stRxmode.type;
			resol = stRxmode.resol;

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			pstCEQData = (_stCEQData *)&pHost->sysHost.stCEQDataList[mapChn];
			pstPortChSel = &pHost->sysHost.portChSel[mapChn];
			prChip = pstPortChSel->prChip;
			prChn = pstPortChSel->prChn;
			i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

			switch(resol)
			{/*{{{*/
				case pr1000_outresol_720x480i60: { type = pr1000_format_SD720; } break;
				case pr1000_outresol_720x576i50: { type = pr1000_format_SD720; } break;
				case pr1000_outresol_960x480i60: { type = pr1000_format_SD960; } break;
				case pr1000_outresol_960x576i50: { type = pr1000_format_SD960; } break;
				default: break;
			}/*}}}*/

			{
				PR1000_VID_LoadTable(fd, &pHost->sysHost.portChSel[mapChn], FALSE, type, resol);

				PR1000_PTZ_LoadTable(fd, &pHost->sysHost.portChSel[mapChn], type, resol, pHost);
				PR1000_PTZ_Init(fd, &pHost->sysHost.portChSel[mapChn], pHost);
				PR1000_PTZ_SetPattern(fd, &pHost->sysHost.portChSel[mapChn], type, resol, pHost);

				PR1000_VEVENT_LoadTable(fd, &pHost->sysHost.portChSel[mapChn], type, resol);

#ifndef DONT_SUPPORT_EVENT_FUNC
				PR1000_VEVENT_ClearMask(fd, &pHost->sysHost.portChSel[mapChn]);
				PR1000_VEVENT_SetMaskAttr(fd, &pHost->sysHost.portChSel[mapChn], resol, &pr1000_mask_attr_table_vevent[resol]);

				/* set md/bd/nd/dd/dfd attribute */
				{
					_stMaskCellAttr stMaskCellAttr;
					/* VEVENT attr */
					_stMdAttr stMdAttr;
					_stBdAttr stBdAttr;
					_stBdSpSens stBdSpSens;
					_stNdAttr stNdAttr;
					_stDdAttr stDdAttr;
					_stDfdAttr stDfdAttr;

					PR1000_VEVENT_GetMaskAttr(fd, &pHost->sysHost.portChSel[mapChn], resol, &stMaskCellAttr);

					/* get default attribute */
					PR1000_VEVENT_GetMdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stMdAttr);
					PR1000_VEVENT_GetBdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stBdAttr);
					PR1000_VEVENT_GetNdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stNdAttr);
					PR1000_VEVENT_GetDdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stDdAttr);
					PR1000_VEVENT_GetDfdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stDfdAttr);

					Dbg("VEVENT Set Md/Bd/Nd/Dd/Dfd [mapChn:%d]\n", mapChn);
					PR1000_VEVENT_SetMdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stMdAttr);
					stBdSpSens.spsens = ((stMaskCellAttr.cellCntX * stMaskCellAttr.cellCntY)/100)*50; //Set 50% blind.
					PR1000_VEVENT_SetBdSpSens(fd, &pHost->sysHost.portChSel[mapChn], &stBdSpSens);
					PR1000_VEVENT_SetBdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stBdAttr);
					PR1000_VEVENT_SetNdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stNdAttr);
					PR1000_VEVENT_SetDdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stDdAttr);
					PR1000_VEVENT_SetDfdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stDfdAttr);

					PR1000_VEVENT_SetDisplayCellFormat(fd, &pHost->sysHost.portChSel[mapChn], &stDefVEVENTDisplayAttr);
					memcpy(&pHost->eventHost.stVEVENTDisplayAttr[mapChn], &stDefVEVENTDisplayAttr, sizeof(_stVEVENTDisplayAttr));
					pHost->eventHost.stVEVENTDisplayAttr[mapChn].mapChn = mapChn;
				}
#endif // DONT_SUPPORT_EVENT_FUNC
			}    

			/* set manual mode register. Do after load vid table. */
			_SET_BIT(mapChn, &pHost->sysHost.bitChnLoadedRegTable); //indicate loading table chnnel.

			pstCEQData->eqProcFlag.AC_GAIN_ADJ = 0;
			pstCEQData->eqProcFlag.EQ_CNT = 1;
			pstCEQData->eqProcFlag.AC_GAIN_HOLD = 0;

			// Check chroma lock when (C_DET = 1 && C_LOCK = 0).
			{/*{{{*/
				i2cReg = 0x01 + PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);
				PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_COMMON, i2cReg, &stCeqLock.reg);
				Dbg("mapChn:%d, chroma det:%d, lock:%d]\n", mapChn COMMA stCeqLock.b.det_chroma COMMA stCeqLock.b.lock_chroma);
				if(stCeqLock.b.det_chroma == 0)
				{
					mdelay(300); //wait a moment one time.
				}
				PR1000_PageRead(fd, i2cSlaveAddr, PR1000_REG_PAGE_COMMON, i2cReg, &stCeqLock.reg);
				Dbg("mapChn:%d, chroma det:%d, lock:%d]\n", mapChn COMMA stCeqLock.b.det_chroma COMMA stCeqLock.b.lock_chroma);

				if( ((stCeqLock.b.det_chroma == 1) && (stCeqLock.b.lock_chroma == 1)) ) 
				{
					_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock); //indicate check chroma lock.
					_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnDoneChromaLock);
					_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultChromaLock);
					pstCEQData->eqProcFlag.C_LOCK_CNT = 1;
					pstCEQData->monStep = PR1000_MON_STEP_START;
					Print("--> Locked chroma. [mapChn:%d, cnt:%d]\n", mapChn COMMA pHost->sysHost.cntChromaLockTunn[mapChn]);

					/* immediately start eq tunning. Do after load vid table. */
					if(_TEST_BIT(mapChn, &pHost->sysHost.bitChnCheckVadcGainSel) == 0 )
					{
						Print("immediately start eq tunning. after rxmode. mapChn:%d\n", mapChn);
						_SET_BIT(mapChn, &pHost->sysHost.bitChnWakeIsrImmediately);
					}
				}
				else if( ((stCeqLock.b.det_chroma == 1) && (stCeqLock.b.lock_chroma == 0)) ) 
				{
					pHost->sysHost.cntChromaLockTunn[mapChn] = 0;
					_SET_BIT(mapChn, &pHost->sysHost.bitChnCheckChromaLock); //indicate check chroma lock.
					_SET_BIT(mapChn, &pHost->sysHost.bitChnWaitCheckChromaLock); //indicate check chroma lock.
					_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnDoneChromaLock);
					_CLEAR_BIT(mapChn, &pHost->sysHost.bitChnResultChromaLock);
					Print("Search chroma phase. mapChn:%d\n", mapChn);
				}
				else
				{
					pstCEQData->monStep = PR1000_MON_STEP_START;
				}
			}/*}}}*/

#if 0 //debug
			{
				uint8_t i2cReg = 0;
				uint16_t i2cData = 0;
				uint8_t i2cMask = 0;
				int page;
				int i;

				page = PR1000_REG_PAGE_COMMON;
				i2cReg = 0x80;
				i2cData = 0x80;
				PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData);

				for(i = 0; i < 4; i++)
				{
					page = PR1000_REG_PAGE_COMMON;
					i2cReg = 0x11 + PR1000_OFFSETADDR_CEQ_INFO_CH(i);
					i2cMask = 0xF0; i2cData = 0x70;
					PR1000_WriteMaskBit(fd, i2cSlaveAddr, page, i2cReg, i2cMask, i2cData);
					i2cReg = 0x12 + PR1000_OFFSETADDR_CEQ_INFO_CH(i);
					i2cData = 0x38;
					PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData);
					i2cReg = 0x13 + PR1000_OFFSETADDR_CEQ_INFO_CH(i);
					i2cData = 0x05;
					PR1000_PageWrite(fd, i2cSlaveAddr, page, i2cReg, i2cData);
				}
			}
#endif

		}/*}}}*/
		break;
		case PR_IOG_RXMODE:
		{/*{{{*/
			int mapChn;
			_stPrRxMode stRxmode;

			if( (ret = copy_from_user((void *)&stRxmode, pArg, sizeof(_stPrRxMode))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
			Dbg("PR_IOG_RXMODE(mapChn:%d)\n", stRxmode.mapChn);

			mapChn = stRxmode.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			stRxmode.mapChn = mapChn;
			stRxmode.type = pHost->sysHost.gPR1000RxType[mapChn];
			stRxmode.resol = pHost->sysHost.gPR1000RxResol[mapChn];

			if( (ret = copy_to_user( pArg, &stRxmode, sizeof(_stPrRxMode))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
			Dbg("get rxmode mapChn:%d, type:%d(%s), resol:%d(%s)\n", 
					stRxmode.mapChn COMMA 
					stRxmode.type COMMA _STR_PR1000_FORMAT[stRxmode.type] COMMA
					stRxmode.resol COMMA _STR_PR1000_OUTRESOL[stRxmode.resol]);
		}/*}}}*/
		break;
#ifndef DONT_SUPPORT_ETC_FUNC
		case PR_IOS_PWDOWN:
		{/*{{{*/
			_stPrPwDown stPrPwDown;

			if( (ret = copy_from_user((void *)&stPrPwDown, pArg, sizeof(_stPrPwDown))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
			DbgString("PR_IOS_PWDOWN\n");

			if( (ret = PR1000_SetPwDown(fd, &stPrPwDown)) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOG_PWDOWN:
		{/*{{{*/
			_stPrPwDown stPrPwDown;

			if( (ret = copy_from_user((void *)&stPrPwDown, pArg, sizeof(_stPrPwDown))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
			DbgString("PR_IOG_PWDOWN\n");

			if( (ret = PR1000_GetPwDown(fd, &stPrPwDown)) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			if( (ret = copy_to_user( pArg, &stPrPwDown, sizeof(_stPrPwDown))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
#endif // DONT_SUPPORT_ETC_FUNC
		case PR_IOS_VID_CHNATTR:
		{/*{{{*/
			int mapChn;
			_stChnAttr stChnAttr;

			if( (ret = copy_from_user((void *)&stChnAttr, pArg, sizeof(_stChnAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_VID_CHNATTR(mapChn:%d)\n", stChnAttr.mapChn);

			mapChn = stChnAttr.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VID_SetChnAttr(fd, &pHost->sysHost.portChSel[mapChn], &stChnAttr) < 0)
			{
				ErrorString("PR1000_VID_SetChnAttr\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_VID_CHNATTR:
		{/*{{{*/
			int mapChn;
			_stChnAttr stChnAttr;

			if( (ret = copy_from_user((void *)&stChnAttr, pArg, sizeof(_stChnAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_VID_CHNATTR(mapChn:%d)\n", stChnAttr.mapChn);

			mapChn = stChnAttr.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VID_GetChnAttr(fd, &pHost->sysHost.portChSel[mapChn], &stChnAttr) < 0)
			{
				ErrorString("PR1000_VID_GetChnAttr\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stChnAttr, sizeof(_stChnAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
#ifndef DONT_SUPPORT_VID_ENHANCEMENT
		case PR_IOS_VID_CSCATTR:
		{/*{{{*/
			int mapChn;
			_stCscAttr stCscAttr;

			if( (ret = copy_from_user((void *)&stCscAttr, pArg, sizeof(_stCscAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_VID_CSCATTR(mapChn:%d)\n", stCscAttr.mapChn);

			mapChn = stCscAttr.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VID_SetCscAttr(fd, &pHost->sysHost.portChSel[mapChn], &stCscAttr) < 0)
			{
				ErrorString("PR1000_VID_SetCscAttr\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_VID_CSCATTR:
		{/*{{{*/
			int mapChn;
			_stCscAttr stCscAttr;

			if( (ret = copy_from_user((void *)&stCscAttr, pArg, sizeof(_stCscAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_VID_CSCATTR(mapChn:%d)\n", stCscAttr.mapChn);

			mapChn = stCscAttr.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VID_GetCscAttr(fd, &pHost->sysHost.portChSel[mapChn], &stCscAttr) < 0)
			{
				ErrorString("PR1000_VID_GetCscAttr\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stCscAttr, sizeof(_stCscAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_VID_CONTRAST:
		{/*{{{*/
			int mapChn;
			_stContrast stContrast;

			if( (ret = copy_from_user((void *)&stContrast, pArg, sizeof(_stContrast))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_VID_CONTRAST(mapChn:%d)\n", stContrast.mapChn);

			mapChn = stContrast.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VID_SetContrast(fd, &pHost->sysHost.portChSel[mapChn], &stContrast) < 0)
			{
				ErrorString("PR1000_VID_SetContrast\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_VID_CONTRAST:
		{/*{{{*/
			int mapChn;
			_stContrast stContrast;

			if( (ret = copy_from_user((void *)&stContrast, pArg, sizeof(_stContrast))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_VID_CONTRAST(mapChn:%d)\n", stContrast.mapChn);

			mapChn = stContrast.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VID_GetContrast(fd, &pHost->sysHost.portChSel[mapChn], &stContrast) < 0)
			{
				ErrorString("PR1000_VID_GetContrast\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stContrast, sizeof(_stContrast))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_VID_BRIGHT:
		{/*{{{*/
			int mapChn;
			_stBright stBright;

			if( (ret = copy_from_user((void *)&stBright, pArg, sizeof(_stBright))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_VID_BRIGHT(mapChn:%d)\n", stBright.mapChn);

			mapChn = stBright.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VID_SetBright(fd, &pHost->sysHost.portChSel[mapChn], &stBright) < 0)
			{
				ErrorString("PR1000_VID_SetBright\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_VID_BRIGHT:
		{/*{{{*/
			int mapChn;
			_stBright stBright;

			if( (ret = copy_from_user((void *)&stBright, pArg, sizeof(_stBright))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_VID_BRIGHT(mapChn:%d)\n", stBright.mapChn);

			mapChn = stBright.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VID_GetBright(fd, &pHost->sysHost.portChSel[mapChn], &stBright) < 0)
			{
				ErrorString("PR1000_VID_GetBright\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stBright, sizeof(_stBright))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_VID_SATURATION:
		{/*{{{*/
			int mapChn;
			_stSaturation stSaturation;

			if( (ret = copy_from_user((void *)&stSaturation, pArg, sizeof(_stSaturation))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_VID_SATURATION(mapChn:%d)\n", stSaturation.mapChn);

			mapChn = stSaturation.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VID_SetSaturation(fd, &pHost->sysHost.portChSel[mapChn], &stSaturation) < 0)
			{
				ErrorString("PR1000_VID_SetSaturation\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_VID_SATURATION:
		{/*{{{*/
			int mapChn;
			_stSaturation stSaturation;

			if( (ret = copy_from_user((void *)&stSaturation, pArg, sizeof(_stSaturation))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_VID_SATURATION(mapChn:%d)\n", stSaturation.mapChn);

			mapChn = stSaturation.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VID_GetSaturation(fd, &pHost->sysHost.portChSel[mapChn], &stSaturation) < 0)
			{
				ErrorString("PR1000_VID_GetSaturation\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stSaturation, sizeof(_stSaturation))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_VID_HUE:
		{/*{{{*/
			int mapChn;
			_stHue stHue;

			if( (ret = copy_from_user((void *)&stHue, pArg, sizeof(_stHue))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_VID_HUE(mapChn:%d)\n", stHue.mapChn);

			mapChn = stHue.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VID_SetHue(fd, &pHost->sysHost.portChSel[mapChn], &stHue) < 0)
			{
				ErrorString("PR1000_VID_SetHue\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_VID_HUE:
		{/*{{{*/
			int mapChn;
			_stHue stHue;

			if( (ret = copy_from_user((void *)&stHue, pArg, sizeof(_stHue))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_VID_HUE(mapChn:%d)\n", stHue.mapChn);

			mapChn = stHue.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VID_GetHue(fd, &pHost->sysHost.portChSel[mapChn], &stHue) < 0)
			{
				ErrorString("PR1000_VID_GetHue\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stHue, sizeof(_stHue))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_VID_SHARPNESS:
		{/*{{{*/
			int mapChn;
			_stSharpness stSharpness;

			if( (ret = copy_from_user((void *)&stSharpness, pArg, sizeof(_stSharpness))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_VID_SHARPNESS(mapChn:%d)\n", stSharpness.mapChn);

			mapChn = stSharpness.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VID_SetSharpness(fd, &pHost->sysHost.portChSel[mapChn], &stSharpness) < 0)
			{
				ErrorString("PR1000_VID_SetSharpness\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_VID_SHARPNESS:
		{/*{{{*/
			int mapChn;
			_stSharpness stSharpness;

			if( (ret = copy_from_user((void *)&stSharpness, pArg, sizeof(_stSharpness))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_VID_SHARPNESS(mapChn:%d)\n", stSharpness.mapChn);

			mapChn = stSharpness.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VID_GetSharpness(fd, &pHost->sysHost.portChSel[mapChn], &stSharpness) < 0)
			{
				ErrorString("PR1000_VID_GetSharpness\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stSharpness, sizeof(_stSharpness))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
#endif // DONT_SUPPORT_VID_ENHANCEMENT
                case PR_IOG_VID_OUTFORMATATTR:
                {/*{{{*/
                        _stOutFormatAttr stOutFormatAttr;

                        DbgString("PR_IOG_VID_OUTFORMATATTR\n");

			memcpy(&stOutFormatAttr, &pHost->sysHost.stOutFormatAttr, sizeof(_stOutFormatAttr));

                        if( (ret = copy_to_user( pArg, &stOutFormatAttr, sizeof(_stOutFormatAttr))) < 0) 
                        {    
                                return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
                        }    
                }/*}}}*/
                break;
		case PR_IOS_VID_OUTCHN:
		{/*{{{*/
			uint8_t prChip, prChn;
			_stOutChn stOutChn;

			if( (ret = copy_from_user((void *)&stOutChn, pArg, sizeof(_stOutChn))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_VID_OUTCHN(prChip:%d)\n", stOutChn.prChip);

			prChip = stOutChn.prChip;
			prChn = stOutChn.prChn;
			if( (prChip >= (PR1000_CHIP_COUNT)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_DEVID));
			}

			if( PR1000_VID_SetOutChn(fd, prChip, &stOutChn) < 0)
			{
				ErrorString("PR1000_VID_SetChn\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
#ifndef DONT_SUPPORT_AUD_ALINK
		case PR_IOS_AUD_AIGAIN:
		{/*{{{*/
			uint8_t prChn = 0;
			uint8_t prChip = 0;
			_stAUDAiGain stAUDAiGain;

			if( (ret = copy_from_user((void *)&stAUDAiGain, pArg, sizeof(_stAUDAiGain))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_AUD_AIGAIN(prChip:%d, prChn:%d)\n", stAUDAiGain.prChip COMMA stAUDAiGain.prChn);

			prChip = stAUDAiGain.prChip;
			if( (prChip >= (MAX_PR1000_CHIP)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_DEVID));
			}
			prChn = stAUDAiGain.prChn;
			if( prChn > 3 )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_DEVID));
			}

			if( PR1000_AUD_SetAiGain(fd, prChip, &stAUDAiGain) < 0)
			{
				ErrorString("PR1000_AUD_SetAiGain\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_AUD_AIGAIN:
		{/*{{{*/
			uint8_t prChn = 0;
			uint8_t prChip = 0;
			_stAUDAiGain stAUDAiGain;

			if( (ret = copy_from_user((void *)&stAUDAiGain, pArg, sizeof(_stAUDAiGain))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_AUD_AIGAIN(prChip:%d)\n", stAUDAiGain.prChip);

			prChip = stAUDAiGain.prChip;
			if( (prChip >= (MAX_PR1000_CHIP)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_DEVID));
			}
			prChn = stAUDAiGain.prChn;
			if( prChn > 3 )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_DEVID));
			}


			if( PR1000_AUD_GetAiGain(fd, prChip, &stAUDAiGain) < 0)
			{
				ErrorString("PR1000_AUD_GetAiGain\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stAUDAiGain, sizeof(_stAUDAiGain))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_AUD_DACGAIN:
		{/*{{{*/
			uint8_t prChip = 0;
			_stAUDDacGain stAUDDacGain;

			if( (ret = copy_from_user((void *)&stAUDDacGain, pArg, sizeof(_stAUDDacGain))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_AUD_DACGAIN(prChip:%d)\n", stAUDDacGain.prChip);

			prChip = stAUDDacGain.prChip;
			if( (prChip >= (MAX_PR1000_CHIP)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_DEVID));
			}

			if( PR1000_AUD_SetDacGain(fd, prChip, &stAUDDacGain) < 0)
			{
				ErrorString("PR1000_AUD_SetDacGain\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_AUD_DACGAIN:
		{/*{{{*/
			uint8_t prChip = 0;
			_stAUDDacGain stAUDDacGain;

			if( (ret = copy_from_user((void *)&stAUDDacGain, pArg, sizeof(_stAUDDacGain))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_AUD_DACGAIN(prChip:%d)\n", stAUDDacGain.prChip);

			prChip = stAUDDacGain.prChip;
			if( (prChip >= (MAX_PR1000_CHIP)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_DEVID));
			}

			if( PR1000_AUD_GetDacGain(fd, prChip, &stAUDDacGain) < 0)
			{
				ErrorString("PR1000_AUD_GetDacGain\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stAUDDacGain, sizeof(_stAUDDacGain))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_AUD_MIXMODE:
		{/*{{{*/
			uint8_t prChip = 0;
			_stAUDMixMode stAUDMixMode;

			if( (ret = copy_from_user((void *)&stAUDMixMode, pArg, sizeof(_stAUDMixMode))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_AUD_MIXMODE(prChip:%d)\n", stAUDMixMode.prChip);

			prChip = stAUDMixMode.prChip;
			if( (prChip >= (MAX_PR1000_CHIP)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_DEVID));
			}

			if( PR1000_AUD_SetMixMode(fd, prChip, &stAUDMixMode) < 0)
			{
				ErrorString("PR1000_AUD_SetMixMode\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_AUD_MIXMODE:
		{/*{{{*/
			uint8_t prChip = 0;
			_stAUDMixMode stAUDMixMode;

			if( (ret = copy_from_user((void *)&stAUDMixMode, pArg, sizeof(_stAUDMixMode))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_AUD_MIXMODE(prChip:%d)\n", stAUDMixMode.prChip);

			prChip = stAUDMixMode.prChip;
			if( (prChip >= (MAX_PR1000_CHIP)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_DEVID));
			}

			if( PR1000_AUD_GetMixMode(fd, prChip, &stAUDMixMode) < 0)
			{
				ErrorString("PR1000_AUD_GetMixMode\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stAUDMixMode, sizeof(_stAUDMixMode))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_AUD_RECATTR:
		{/*{{{*/
			uint8_t prChip = 0;
			_stAUDRecAttr stAUDRecAttr;

			if( (ret = copy_from_user((void *)&stAUDRecAttr, pArg, sizeof(_stAUDRecAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_AUD_RECATTR(prChip:%d)\n", stAUDRecAttr.prChip);

			prChip = stAUDRecAttr.prChip;
			if( (prChip >= (MAX_PR1000_CHIP)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_DEVID));
			}

			if( PR1000_AUD_SetRecAttr(fd, prChip, &stAUDRecAttr) < 0)
			{
				ErrorString("PR1000_AUD_SetRecAttr\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_AUD_RECATTR:
		{/*{{{*/
			uint8_t prChip = 0;
			_stAUDRecAttr stAUDRecAttr;

			if( (ret = copy_from_user((void *)&stAUDRecAttr, pArg, sizeof(_stAUDRecAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_AUD_RECATTR(prChip:%d)\n", stAUDRecAttr.prChip);

			prChip = stAUDRecAttr.prChip;
			if( (prChip >= (MAX_PR1000_CHIP)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_DEVID));
			}

			if( PR1000_AUD_GetRecAttr(fd, prChip, &stAUDRecAttr) < 0)
			{
				ErrorString("PR1000_AUD_GetRecAttr\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stAUDRecAttr, sizeof(_stAUDRecAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_AUD_PBATTR:
		{/*{{{*/
			uint8_t prChip = 0;
			_stAUDPbAttr stAUDPbAttr;

			if( (ret = copy_from_user((void *)&stAUDPbAttr, pArg, sizeof(_stAUDPbAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_AUD_PBATTR(prChip:%d)\n", stAUDPbAttr.prChip);

			prChip = stAUDPbAttr.prChip;
			if( (prChip >= (MAX_PR1000_CHIP)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_DEVID));
			}

			if( PR1000_AUD_SetPbAttr(fd, prChip, &stAUDPbAttr) < 0)
			{
				ErrorString("PR1000_AUD_SetPbAttr\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_AUD_PBATTR:
		{/*{{{*/
			uint8_t prChip = 0;
			_stAUDPbAttr stAUDPbAttr;

			if( (ret = copy_from_user((void *)&stAUDPbAttr, pArg, sizeof(_stAUDPbAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_AUD_PBATTR(prChip:%d)\n", stAUDPbAttr.prChip);

			prChip = stAUDPbAttr.prChip;
			if( (prChip >= (MAX_PR1000_CHIP)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_DEVID));
			}

			if( PR1000_AUD_GetPbAttr(fd, prChip, &stAUDPbAttr) < 0)
			{
				ErrorString("PR1000_AUD_GetPbAttr\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stAUDPbAttr, sizeof(_stAUDPbAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_AUD_RECMUTE:
		{/*{{{*/
			uint8_t prChip = 0;
			uint8_t prChn = 0;
			_stAudRecMute stAudRecMute;

			if( (ret = copy_from_user((void *)&stAudRecMute, pArg, sizeof(_stAudRecMute))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_AUD_RECMUTE(prChip:%d, prChn:%d)\n", stAudRecMute.prChip COMMA stAudRecMute.prChn);

			prChip = stAudRecMute.prChip;
			prChn = stAudRecMute.prChn;
			if( (prChip >= (MAX_PR1000_CHIP)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_DEVID));
			}
			if( (prChn >= (MAX_PR1000_CHIP*DEF_PR1000_MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if( PR1000_AUD_SetRecMute(fd, prChip, prChn, stAudRecMute.bEnable) < 0)
			{
				ErrorString("PR1000_AUD_SetRecMute\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

		}/*}}}*/
		break;
		case PR_IOS_AUD_MIXMUTE:
		{/*{{{*/
			uint8_t prChip = 0;
			_stAudMixMute stAudMixMute;

			if( (ret = copy_from_user((void *)&stAudMixMute, pArg, sizeof(_stAudMixMute))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_AUD_MIXMUTE(prChip:%d)\n", stAudMixMute.prChip);

			prChip = stAudMixMute.prChip;
			if( (prChip >= (MAX_PR1000_CHIP)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_DEVID));
			}

			if( PR1000_AUD_SetMixMute(fd, prChip, stAudMixMute.bEnable) < 0)
			{
				ErrorString("PR1000_AUD_SetMixMute\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOS_AUD_VOCMUTE:
		{/*{{{*/
			uint8_t prChip = 0;
			_stAudVocMute stAudVocMute;

			if( (ret = copy_from_user((void *)&stAudVocMute, pArg, sizeof(_stAudVocMute))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_AUD_VOCMUTE(prChip:%d)\n", stAudVocMute.prChip);

			prChip = stAudVocMute.prChip;
			if( (prChip >= (MAX_PR1000_CHIP)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_DEVID));
			}

			if( PR1000_AUD_SetVocMute(fd, prChip, stAudVocMute.bEnable) < 0)
			{
				ErrorString("PR1000_AUD_SetVocMute\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOS_AUD_DACMUTE:
		{/*{{{*/
			uint8_t prChip = 0;
			_stAudDacMute stAudDacMute;

			if( (ret = copy_from_user((void *)&stAudDacMute, pArg, sizeof(_stAudDacMute))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_AUD_DACMUTE(prChip:%d)\n", stAudDacMute.prChip);

			prChip = stAudDacMute.prChip;
			if( (prChip >= (MAX_PR1000_CHIP)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_DEVID));
			}

			if( PR1000_AUD_SetDacMute(fd, prChip, stAudDacMute.bEnable) < 0)
			{
				ErrorString("PR1000_AUD_SetDacMute\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOS_AUD_DACCHN:
		{/*{{{*/
			uint8_t prChip = 0;
			uint8_t prChn = 0;
			_stAudDacChn stAudDacChn;

			if( (ret = copy_from_user((void *)&stAudDacChn, pArg, sizeof(_stAudDacChn))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_AUD_DACCHN(prChip:%d, prChn:%d)\n", stAudDacChn.prChip COMMA stAudDacChn.prChn);

			prChip = stAudDacChn.prChip;
			prChn = stAudDacChn.prChn;
			if( (prChip >= (MAX_PR1000_CHIP)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_DEVID));
			}

			if( PR1000_AUD_SetDacChn(fd, prChip, prChn) < 0)
			{
				ErrorString("PR1000_AUD_SetDacChn\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

		}/*}}}*/
		break;
		case PR_IOS_AUD_DETATTR:
		{/*{{{*/
			uint8_t prChip;
			_stAUDDetAttr stAUDDetAttr;

			if( (ret = copy_from_user((void *)&stAUDDetAttr, pArg, sizeof(_stAUDDetAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_AUD_DETATTR(prChip:%d)\n", stAUDDetAttr.prChip);

			prChip = stAUDDetAttr.prChip;
			if( (prChip >= (MAX_PR1000_CHIP)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_DEVID));
			}

			if( PR1000_AUD_SetDetAttr(fd, prChip, &stAUDDetAttr) < 0)
			{
				ErrorString("PR1000_AUD_SetDetAttr\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_AUD_DETATTR:
		{/*{{{*/
			uint8_t prChip;
			_stAUDDetAttr stAUDDetAttr;

			if( (ret = copy_from_user((void *)&stAUDDetAttr, pArg, sizeof(_stAUDDetAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_AUD_DETATTR(prChip:%d)\n", stAUDDetAttr.prChip);

			prChip = stAUDDetAttr.prChip;
			if( (prChip >= (MAX_PR1000_CHIP)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_DEVID));
			}

			if( PR1000_AUD_GetDetAttr(fd, prChip, &stAUDDetAttr) < 0)
			{
				ErrorString("PR1000_AUD_GetDetAttr\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stAUDDetAttr, sizeof(_stAUDDetAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
#endif // DONT_SUPPORT_AUD_ALINK
#ifndef DONT_SUPPORT_EVENT_FUNC
		case PR_IOS_EVENT_MDATTR:
		{/*{{{*/
			int mapChn = 0;
			_stMdAttr stMdAttr;

			if( (ret = copy_from_user((void *)&stMdAttr, pArg, sizeof(_stMdAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_EVENT_MDATTR(mapChn:%d)\n", stMdAttr.mapChn);

			mapChn = stMdAttr.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_SetMdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stMdAttr) < 0)
			{
				ErrorString("PR1000_VEVENT_SetMdAttr\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_EVENT_MDATTR:
		{/*{{{*/
			int mapChn = 0;
			_stMdAttr stMdAttr;

			if( (ret = copy_from_user((void *)&stMdAttr, pArg, sizeof(_stMdAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_MDATTR(mapChn:%d)\n", stMdAttr.mapChn);

			mapChn = stMdAttr.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_GetMdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stMdAttr) < 0)
			{
				ErrorString("PR1000_VEVENT_GetMdAttr\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stMdAttr, sizeof(_stMdAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_EVENT_MD_LVSENS:
		{/*{{{*/
			int mapChn = 0;
			_stMdLvSens stMdLvSens;

			if( (ret = copy_from_user((void *)&stMdLvSens, pArg, sizeof(_stMdLvSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_EVENT_MD_LVSENS(mapChn:%d)\n", stMdLvSens.mapChn);

			mapChn = stMdLvSens.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_SetMdLvSens(fd, &pHost->sysHost.portChSel[mapChn], &stMdLvSens) < 0)
			{
				ErrorString("PR1000_VEVENT_SetMdLvSens\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_EVENT_MD_LVSENS:
		{/*{{{*/
			int mapChn = 0;
			_stMdLvSens stMdLvSens;

			if( (ret = copy_from_user((void *)&stMdLvSens, pArg, sizeof(_stMdLvSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_MD_LVSENS(mapChn:%d)\n", stMdLvSens.mapChn);

			mapChn = stMdLvSens.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_GetMdLvSens(fd, &pHost->sysHost.portChSel[mapChn], &stMdLvSens) < 0)
			{
				ErrorString("PR1000_VEVENT_GetMdLvSens\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stMdLvSens, sizeof(_stMdLvSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_EVENT_MD_SPSENS:
		{/*{{{*/
			int mapChn = 0;
			_stMdSpSens stMdSpSens;

			if( (ret = copy_from_user((void *)&stMdSpSens, pArg, sizeof(_stMdSpSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_EVENT_MD_SPSENS(mapChn:%d)\n", stMdSpSens.mapChn);

			mapChn = stMdSpSens.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_SetMdSpSens(fd, &pHost->sysHost.portChSel[mapChn], &stMdSpSens) < 0)
			{
				ErrorString("PR1000_VEVENT_SetMdSpSens\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_EVENT_MD_SPSENS:
		{/*{{{*/
			int mapChn = 0;
			_stMdSpSens stMdSpSens;

			if( (ret = copy_from_user((void *)&stMdSpSens, pArg, sizeof(_stMdSpSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_MD_SPSENS(mapChn:%d)\n", stMdSpSens.mapChn);

			mapChn = stMdSpSens.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_GetMdSpSens(fd, &pHost->sysHost.portChSel[mapChn], &stMdSpSens) < 0)
			{
				ErrorString("PR1000_VEVENT_GetMdSpSens\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stMdSpSens, sizeof(_stMdSpSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_EVENT_MD_TMPSENS:
		{/*{{{*/
			int mapChn = 0;
			_stMdTmpSens stMdTmpSens;

			if( (ret = copy_from_user((void *)&stMdTmpSens, pArg, sizeof(_stMdTmpSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_EVENT_MD_TMPSENS(mapChn:%d)\n", stMdTmpSens.mapChn);

			mapChn = stMdTmpSens.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_SetMdTmpSens(fd, &pHost->sysHost.portChSel[mapChn], &stMdTmpSens) < 0)
			{
				ErrorString("PR1000_VEVENT_SetMdTmpSens\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_EVENT_MD_TMPSENS:
		{/*{{{*/
			int mapChn = 0;
			_stMdTmpSens stMdTmpSens;

			if( (ret = copy_from_user((void *)&stMdTmpSens, pArg, sizeof(_stMdTmpSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_MD_TMPSENS(mapChn:%d)\n", stMdTmpSens.mapChn);

			mapChn = stMdTmpSens.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_GetMdTmpSens(fd, &pHost->sysHost.portChSel[mapChn], &stMdTmpSens) < 0)
			{
				ErrorString("PR1000_VEVENT_GetMdTmpSens\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stMdTmpSens, sizeof(_stMdTmpSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_EVENT_MD_VELOCITY:
		{/*{{{*/
			int mapChn = 0;
			_stMdVelocity stMdVelocity;

			if( (ret = copy_from_user((void *)&stMdVelocity, pArg, sizeof(_stMdVelocity))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_EVENT_MD_VELOCITY(mapChn:%d)\n", stMdVelocity.mapChn);

			mapChn = stMdVelocity.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_SetMdVelocity(fd, &pHost->sysHost.portChSel[mapChn], &stMdVelocity) < 0)
			{
				ErrorString("PR1000_VEVENT_SetMdVelocity\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_EVENT_MD_VELOCITY:
		{/*{{{*/
			int mapChn = 0;
			_stMdVelocity stMdVelocity;

			if( (ret = copy_from_user((void *)&stMdVelocity, pArg, sizeof(_stMdVelocity))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_MD_VELOCITY(mapChn:%d)\n", stMdVelocity.mapChn);

			mapChn = stMdVelocity.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_GetMdVelocity(fd, &pHost->sysHost.portChSel[mapChn], &stMdVelocity) < 0)
			{
				ErrorString("PR1000_VEVENT_GetMdVelocity\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stMdVelocity, sizeof(_stMdVelocity))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_EVENT_BDATTR:
		{/*{{{*/
			int mapChn = 0;
			_stBdAttr stBdAttr;

			if( (ret = copy_from_user((void *)&stBdAttr, pArg, sizeof(_stBdAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_EVENT_BDATTR(mapChn:%d)\n", stBdAttr.mapChn);

			mapChn = stBdAttr.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_SetBdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stBdAttr) < 0)
			{
				ErrorString("PR1000_VEVENT_SetBdAttr\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_EVENT_BDATTR:
		{/*{{{*/
			int mapChn = 0;
			_stBdAttr stBdAttr;

			if( (ret = copy_from_user((void *)&stBdAttr, pArg, sizeof(_stBdAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_BDATTR(mapChn:%d)\n", stBdAttr.mapChn);

			mapChn = stBdAttr.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}


			if( PR1000_VEVENT_GetBdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stBdAttr) < 0)
			{
				ErrorString("PR1000_VEVENT_GetBdAttr\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stBdAttr, sizeof(_stBdAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_EVENT_BD_LVSENS:
		{/*{{{*/
			int mapChn = 0;
			_stBdLvSens stBdLvSens;

			if( (ret = copy_from_user((void *)&stBdLvSens, pArg, sizeof(_stBdLvSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_EVENT_BD_LVSENS(mapChn:%d)\n", stBdLvSens.mapChn);

			mapChn = stBdLvSens.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_SetBdLvSens(fd, &pHost->sysHost.portChSel[mapChn], &stBdLvSens) < 0)
			{
				ErrorString("PR1000_VEVENT_SetBdLvSens\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_EVENT_BD_LVSENS:
		{/*{{{*/
			int mapChn = 0;
			_stBdLvSens stBdLvSens;

			if( (ret = copy_from_user((void *)&stBdLvSens, pArg, sizeof(_stBdLvSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_BD_LVSENS(mapChn:%d)\n", stBdLvSens.mapChn);

			mapChn = stBdLvSens.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_GetBdLvSens(fd, &pHost->sysHost.portChSel[mapChn], &stBdLvSens) < 0)
			{
				ErrorString("PR1000_VEVENT_GetBdLvSens\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stBdLvSens, sizeof(_stBdLvSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_EVENT_BD_SPSENS:
		{/*{{{*/
			int mapChn = 0;
			_stBdSpSens stBdSpSens;

			if( (ret = copy_from_user((void *)&stBdSpSens, pArg, sizeof(_stBdSpSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_EVENT_BD_SPSENS(mapChn:%d)\n", stBdSpSens.mapChn);

			mapChn = stBdSpSens.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_SetBdSpSens(fd, &pHost->sysHost.portChSel[mapChn], &stBdSpSens) < 0)
			{
				ErrorString("PR1000_VEVENT_SetBdSpSens\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_EVENT_BD_SPSENS:
		{/*{{{*/
			int mapChn = 0;
			_stBdSpSens stBdSpSens;

			if( (ret = copy_from_user((void *)&stBdSpSens, pArg, sizeof(_stBdSpSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_BD_SPSENS(mapChn:%d)\n", stBdSpSens.mapChn);

			mapChn = stBdSpSens.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_GetBdSpSens(fd, &pHost->sysHost.portChSel[mapChn], &stBdSpSens) < 0)
			{
				ErrorString("PR1000_VEVENT_GetBdSpSens\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stBdSpSens, sizeof(_stBdSpSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_EVENT_BD_TMPSENS:
		{/*{{{*/
			int mapChn = 0;
			_stBdTmpSens stBdTmpSens;

			if( (ret = copy_from_user((void *)&stBdTmpSens, pArg, sizeof(_stBdTmpSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_EVENT_BD_TMPSENS(mapChn:%d)\n", stBdTmpSens.mapChn);

			mapChn = stBdTmpSens.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_SetBdTmpSens(fd, &pHost->sysHost.portChSel[mapChn], &stBdTmpSens) < 0)
			{
				ErrorString("PR1000_VEVENT_SetBdTmpSens\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_EVENT_BD_TMPSENS:
		{/*{{{*/
			int mapChn = 0;
			_stBdTmpSens stBdTmpSens;

			if( (ret = copy_from_user((void *)&stBdTmpSens, pArg, sizeof(_stBdTmpSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_BD_TMPSENS(mapChn:%d)\n", stBdTmpSens.mapChn);

			mapChn = stBdTmpSens.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_GetBdTmpSens(fd, &pHost->sysHost.portChSel[mapChn], &stBdTmpSens) < 0)
			{
				ErrorString("PR1000_VEVENT_GetBdTmpSens\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stBdTmpSens, sizeof(_stBdTmpSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_EVENT_NDATTR:
		{/*{{{*/
			int mapChn = 0;
			_stNdAttr stNdAttr;

			if( (ret = copy_from_user((void *)&stNdAttr, pArg, sizeof(_stNdAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_EVENT_NDATTR(mapChn:%d)\n", stNdAttr.mapChn);

			mapChn = stNdAttr.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_SetNdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stNdAttr) < 0)
			{
				ErrorString("PR1000_VEVENT_SetNdAttr\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_EVENT_NDATTR:
		{/*{{{*/
			int mapChn = 0;
			_stNdAttr stNdAttr;

			if( (ret = copy_from_user((void *)&stNdAttr, pArg, sizeof(_stNdAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_NDATTR(mapChn:%d)\n", stNdAttr.mapChn);

			mapChn = stNdAttr.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_GetNdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stNdAttr) < 0)
			{
				ErrorString("PR1000_VEVENT_GetNdAttr\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stNdAttr, sizeof(_stNdAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_EVENT_ND_LVSENS:
		{/*{{{*/
			int mapChn = 0;
			_stNdLvSens stNdLvSens;

			if( (ret = copy_from_user((void *)&stNdLvSens, pArg, sizeof(_stNdLvSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_EVENT_ND_LVSENS(mapChn:%d)\n", stNdLvSens.mapChn);

			mapChn = stNdLvSens.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_SetNdLvSens(fd, &pHost->sysHost.portChSel[mapChn], &stNdLvSens) < 0)
			{
				ErrorString("PR1000_VEVENT_SetNdLvSens\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_EVENT_ND_LVSENS:
		{/*{{{*/
			int mapChn = 0;
			_stNdLvSens stNdLvSens;

			if( (ret = copy_from_user((void *)&stNdLvSens, pArg, sizeof(_stNdLvSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_ND_LVSENS(mapChn:%d)\n", stNdLvSens.mapChn);

			mapChn = stNdLvSens.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_GetNdLvSens(fd, &pHost->sysHost.portChSel[mapChn], &stNdLvSens) < 0)
			{
				ErrorString("PR1000_VEVENT_GetNdLvSens\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stNdLvSens, sizeof(_stNdLvSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_EVENT_ND_TMPSENS:
		{/*{{{*/
			int mapChn = 0;
			_stNdTmpSens stNdTmpSens;

			if( (ret = copy_from_user((void *)&stNdTmpSens, pArg, sizeof(_stNdTmpSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_EVENT_ND_TMPSENS(mapChn:%d)\n", stNdTmpSens.mapChn);

			mapChn = stNdTmpSens.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_SetNdTmpSens(fd, &pHost->sysHost.portChSel[mapChn], &stNdTmpSens) < 0)
			{
				ErrorString("PR1000_VEVENT_SetNdTmpSens\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_EVENT_ND_TMPSENS:
		{/*{{{*/
			int mapChn = 0;
			_stNdTmpSens stNdTmpSens;

			if( (ret = copy_from_user((void *)&stNdTmpSens, pArg, sizeof(_stNdTmpSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_ND_TMPSENS(mapChn:%d)\n", stNdTmpSens.mapChn);

			mapChn = stNdTmpSens.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_GetNdTmpSens(fd, &pHost->sysHost.portChSel[mapChn], &stNdTmpSens) < 0)
			{
				ErrorString("PR1000_VEVENT_GetNdTmpSens\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stNdTmpSens, sizeof(_stNdTmpSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_EVENT_DDATTR:
		{/*{{{*/
			int mapChn = 0;
			_stDdAttr stDdAttr;

			if( (ret = copy_from_user((void *)&stDdAttr, pArg, sizeof(_stDdAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_EVENT_DDATTR(mapChn:%d)\n", stDdAttr.mapChn);

			mapChn = stDdAttr.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_SetDdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stDdAttr) < 0)
			{
				ErrorString("PR1000_VEVENT_SetDdAttr\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_EVENT_DDATTR:
		{/*{{{*/
			int mapChn = 0;
			_stDdAttr stDdAttr;

			if( (ret = copy_from_user((void *)&stDdAttr, pArg, sizeof(_stDdAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_DDATTR(mapChn:%d)\n", stDdAttr.mapChn);

			mapChn = stDdAttr.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_GetDdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stDdAttr) < 0)
			{
				ErrorString("PR1000_VEVENT_GetDdAttr\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stDdAttr, sizeof(_stDdAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_EVENT_DD_LVSENS:
		{/*{{{*/
			int mapChn = 0;
			_stDdLvSens stDdLvSens;

			if( (ret = copy_from_user((void *)&stDdLvSens, pArg, sizeof(_stDdLvSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_EVENT_DD_LVSENS(mapChn:%d)\n", stDdLvSens.mapChn);

			mapChn = stDdLvSens.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_SetDdLvSens(fd, &pHost->sysHost.portChSel[mapChn], &stDdLvSens) < 0)
			{
				ErrorString("PR1000_VEVENT_SetDdLvSens\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_EVENT_DD_LVSENS:
		{/*{{{*/
			int mapChn = 0;
			_stDdLvSens stDdLvSens;

			if( (ret = copy_from_user((void *)&stDdLvSens, pArg, sizeof(_stDdLvSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_DD_LVSENS(mapChn:%d)\n", stDdLvSens.mapChn);

			mapChn = stDdLvSens.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_GetDdLvSens(fd, &pHost->sysHost.portChSel[mapChn], &stDdLvSens) < 0)
			{
				ErrorString("PR1000_VEVENT_GetDdLvSens\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stDdLvSens, sizeof(_stDdLvSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_EVENT_DD_TMPSENS:
		{/*{{{*/
			int mapChn = 0;
			_stDdTmpSens stDdTmpSens;

			if( (ret = copy_from_user((void *)&stDdTmpSens, pArg, sizeof(_stDdTmpSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_EVENT_DD_TMPSENS(mapChn:%d)\n", stDdTmpSens.mapChn);

			mapChn = stDdTmpSens.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_SetDdTmpSens(fd, &pHost->sysHost.portChSel[mapChn], &stDdTmpSens) < 0)
			{
				ErrorString("PR1000_VEVENT_SetDdTmpSens\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_EVENT_DD_TMPSENS:
		{/*{{{*/
			int mapChn = 0;
			_stDdTmpSens stDdTmpSens;

			if( (ret = copy_from_user((void *)&stDdTmpSens, pArg, sizeof(_stDdTmpSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_DD_TMPSENS(mapChn:%d)\n", stDdTmpSens.mapChn);

			mapChn = stDdTmpSens.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_GetDdTmpSens(fd, &pHost->sysHost.portChSel[mapChn], &stDdTmpSens) < 0)
			{
				ErrorString("PR1000_VEVENT_GetDdTmpSens\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stDdTmpSens, sizeof(_stDdTmpSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_EVENT_DFDATTR:
		{/*{{{*/
			int mapChn = 0;
			_stDfdAttr stDfdAttr;

			if( (ret = copy_from_user((void *)&stDfdAttr, pArg, sizeof(_stDfdAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_EVENT_DFDATTR(mapChn:%d)\n", stDfdAttr.mapChn);

			mapChn = stDfdAttr.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_SetDfdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stDfdAttr) < 0)
			{
				ErrorString("PR1000_VEVENT_SetDfdAttr\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_EVENT_DFDATTR:
		{/*{{{*/
			int mapChn = 0;
			_stDfdAttr stDfdAttr;

			if( (ret = copy_from_user((void *)&stDfdAttr, pArg, sizeof(_stDfdAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_DFDATTR(mapChn:%d)\n", stDfdAttr.mapChn);

			mapChn = stDfdAttr.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_GetDfdAttr(fd, &pHost->sysHost.portChSel[mapChn], &stDfdAttr) < 0)
			{
				ErrorString("PR1000_VEVENT_GetDfdAttr\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stDfdAttr, sizeof(_stDfdAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_EVENT_DFD_LVSENS:
		{/*{{{*/
			int mapChn = 0;
			_stDfdLvSens stDfdLvSens;

			if( (ret = copy_from_user((void *)&stDfdLvSens, pArg, sizeof(_stDfdLvSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_EVENT_DFD_LVSENS(mapChn:%d)\n", stDfdLvSens.mapChn);

			mapChn = stDfdLvSens.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_SetDfdLvSens(fd, &pHost->sysHost.portChSel[mapChn], &stDfdLvSens) < 0)
			{
				ErrorString("PR1000_VEVENT_SetDfdLvSens\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_EVENT_DFD_LVSENS:
		{/*{{{*/
			int mapChn = 0;
			_stDfdLvSens stDfdLvSens;

			if( (ret = copy_from_user((void *)&stDfdLvSens, pArg, sizeof(_stDfdLvSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_DFD_LVSENS(mapChn:%d)\n", stDfdLvSens.mapChn);

			mapChn = stDfdLvSens.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_GetDfdLvSens(fd, &pHost->sysHost.portChSel[mapChn], &stDfdLvSens) < 0)
			{
				ErrorString("PR1000_VEVENT_GetDfdLvSens\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stDfdLvSens, sizeof(_stDfdLvSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_EVENT_DFD_SPSENS:
		{/*{{{*/
			int mapChn = 0;
			_stDfdSpSens stDfdSpSens;

			if( (ret = copy_from_user((void *)&stDfdSpSens, pArg, sizeof(_stDfdSpSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_EVENT_DFD_SPSENS(mapChn:%d)\n", stDfdSpSens.mapChn);

			mapChn = stDfdSpSens.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_SetDfdSpSens(fd, &pHost->sysHost.portChSel[mapChn], &stDfdSpSens) < 0)
			{
				ErrorString("PR1000_VEVENT_SetDfdSpSens\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_EVENT_DFD_SPSENS:
		{/*{{{*/
			int mapChn = 0;
			_stDfdSpSens stDfdSpSens;

			if( (ret = copy_from_user((void *)&stDfdSpSens, pArg, sizeof(_stDfdSpSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_DFD_SPSENS(mapChn:%d)\n", stDfdSpSens.mapChn);

			mapChn = stDfdSpSens.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_GetDfdSpSens(fd, &pHost->sysHost.portChSel[mapChn], &stDfdSpSens) < 0)
			{
				ErrorString("PR1000_VEVENT_GetDfdSpSens\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stDfdSpSens, sizeof(_stDfdSpSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_EVENT_DFD_TMPSENS:
		{/*{{{*/
			int mapChn = 0;
			_stDfdTmpSens stDfdTmpSens;

			if( (ret = copy_from_user((void *)&stDfdTmpSens, pArg, sizeof(_stDfdTmpSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_EVENT_DFD_TMPSENS(mapChn:%d)\n", stDfdTmpSens.mapChn);

			mapChn = stDfdTmpSens.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_SetDfdTmpSens(fd, &pHost->sysHost.portChSel[mapChn], &stDfdTmpSens) < 0)
			{
				ErrorString("PR1000_VEVENT_SetDfdTmpSens\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_EVENT_DFD_TMPSENS:
		{/*{{{*/
			int mapChn = 0;
			_stDfdTmpSens stDfdTmpSens;

			if( (ret = copy_from_user((void *)&stDfdTmpSens, pArg, sizeof(_stDfdTmpSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_DFD_TMPSENS(mapChn:%d)\n", stDfdTmpSens.mapChn);

			mapChn = stDfdTmpSens.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_GetDfdTmpSens(fd, &pHost->sysHost.portChSel[mapChn], &stDfdTmpSens) < 0)
			{
				ErrorString("PR1000_VEVENT_GetDfdTmpSens\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stDfdTmpSens, sizeof(_stDfdTmpSens))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_EVENT_MASKCELL_SIZE:
		{/*{{{*/
			int mapChn = 0;
			_stVEVENTMaskCellSize stVEVENTMaskCellSize;
			_stMaskCellAttr stMaskCellAttr;
			int maxH = 0, maxV = 0;
			int dispMaxH = 0, dispMaxV = 0;

			if( (ret = copy_from_user((void *)&stVEVENTMaskCellSize, pArg, sizeof(_stVEVENTMaskCellSize))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_EVENT_MASKCELL_SIZE(mapChn:%d)\n", stVEVENTMaskCellSize.mapChn);

			mapChn = stVEVENTMaskCellSize.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			/* check max W/H */
			switch(pHost->sysHost.gPR1000RxResol[mapChn])
			{/*{{{*/
				case pr1000_outresol_720x480i60:
					{
						maxH = 704;
						maxV = 480/2; // If interlace camera, half.
						dispMaxH = maxH;
						dispMaxV = maxV;
					}
					break;
				case pr1000_outresol_720x576i50:
					{
						maxH = 704;
						maxV = 576/2; // If interlace camera, half.
						dispMaxH = maxH;
						dispMaxV = maxV;
					}
					break;
				case pr1000_outresol_960x480i60:
					{
						maxH = 960;
						maxV = 480/2; // If interlace camera, half.
						dispMaxH = maxH;
						dispMaxV = maxV;
					}
					break;
				case pr1000_outresol_960x576i50:
					{
						maxH = 960;
						maxV = 576/2; // If interlace camera, half.
						dispMaxH = maxH;
						dispMaxV = maxV;
					}
					break;
				case pr1000_outresol_1280x720p60:
					{
						maxH = 1280;
						maxV = 720;
						dispMaxH = maxH;
						dispMaxV = maxV;
					}
					break;
				case pr1000_outresol_1280x720p60c: //640x720p60
					{
						maxH = 1280;
						maxV = 720;
						dispMaxH = 640;
						dispMaxV = 720;
					}
					break;
				case pr1000_outresol_1280x720p50:
					{
						maxH = 1280;
						maxV = 720;
						dispMaxH = maxH;
						dispMaxV = maxV;
					}
					break;
				case pr1000_outresol_1280x720p50c: //640x720p50
					{
						maxH = 1280;
						maxV = 720;
						dispMaxH = 640;
						dispMaxV = 720;
					}
					break;
				case pr1000_outresol_1280x720p30:
					{
						maxH = 1280;
						maxV = 720;
						dispMaxH = maxH;
						dispMaxV = maxV;
					}
					break;
				case pr1000_outresol_1280x720p30c: //640x720p30
					{
						maxH = 1280;
						maxV = 720;
						dispMaxH = 640;
						dispMaxV = 720;
					}
					break;
				case pr1000_outresol_1280x720p25:
					{
						maxH = 1280;
						maxV = 720;
						dispMaxH = maxH;
						dispMaxV = maxV;
					}
					break;
				case pr1000_outresol_1280x720p25c: //640x720p25
					{
						maxH = 1280;
						maxV = 720;
						dispMaxH = 640;
						dispMaxV = 720;
					}
					break;
				case pr1000_outresol_1920x1080p30:
					{
						maxH = 1920;
						maxV = 1080;
						dispMaxH = maxH;
						dispMaxV = maxV;
					}
					break;
				case pr1000_outresol_1920x1080p30c: //960x1080p30
					{
						maxH = 1920;
						maxV = 1080;
						dispMaxH = 960;
						dispMaxV = 1080;
					}
					break;
				case pr1000_outresol_1920x1080p30s: //1280x720p30
					{
						maxH = 1920;
						maxV = 1080;
						dispMaxH = 1280;
						dispMaxV = 720;
					}
					break;
				case pr1000_outresol_1920x1080p25:
					{
						maxH = 1920;
						maxV = 1080;
						dispMaxH = maxH;
						dispMaxV = maxV;
					}
					break;
				case pr1000_outresol_1920x1080p25c: //960x1080p25
					{
						maxH = 1920;
						maxV = 1080;
						dispMaxH = 960;
						dispMaxV = 1080;
					}
					break;
				case pr1000_outresol_1920x1080p25s: //1280x720p25
					{
						maxH = 1920;
						maxV = 1080;
						dispMaxH = 1280;
						dispMaxV = 720;
					}
					break;
				default:
					{
						return(-1);
					}
					break;
			}/*}}}*/

			if(dispMaxH < (stVEVENTMaskCellSize.cellHStartOffset + (stVEVENTMaskCellSize.cellCntX * stVEVENTMaskCellSize.cellWidth)) )
			{
				Error("Wrong md width(mapChn:%d off:%d, cntx:%d, width:%d)\n", mapChn COMMA stVEVENTMaskCellSize.cellHStartOffset COMMA stVEVENTMaskCellSize.cellCntX COMMA stVEVENTMaskCellSize.cellWidth);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
			else if(dispMaxV < (stVEVENTMaskCellSize.cellVStartOffset + (stVEVENTMaskCellSize.cellCntY * stVEVENTMaskCellSize.cellHeight)) )
			{
				Error("Wrong md height(mapChn:%d off:%d, cnty:%d, height:%d)\n", mapChn COMMA stVEVENTMaskCellSize.cellVStartOffset COMMA stVEVENTMaskCellSize.cellCntY COMMA stVEVENTMaskCellSize.cellHeight);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
			if( (maxH == 0) || (maxV == 0) || (dispMaxH == 0) || (dispMaxV == 0) )
			{
				Error("Invalid argu(mapChn:%d maxH:%d, maxV:%d)\n", mapChn COMMA maxH COMMA maxV);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			stMaskCellAttr.cellCntX = stVEVENTMaskCellSize.cellCntX;
			stMaskCellAttr.cellWidth = stVEVENTMaskCellSize.cellWidth;
			stMaskCellAttr.cellHStartOffset = stVEVENTMaskCellSize.cellHStartOffset;
			stMaskCellAttr.cellCntY = stVEVENTMaskCellSize.cellCntY;
			stMaskCellAttr.cellHeight = stVEVENTMaskCellSize.cellHeight;
			stMaskCellAttr.cellVStartOffset = stVEVENTMaskCellSize.cellVStartOffset;

			Print("md width(mapChn:%d off:%d, cntx:%d, width:%d)\n", mapChn COMMA 
					stMaskCellAttr.cellHStartOffset COMMA 
					stMaskCellAttr.cellCntX COMMA 
					stMaskCellAttr.cellWidth);

			Print("md height(mapChn:%d off:%d, cnty:%d, height:%d)\n", mapChn COMMA 
					stMaskCellAttr.cellVStartOffset COMMA 
					stMaskCellAttr.cellCntY COMMA 
					stMaskCellAttr.cellHeight);

			if( PR1000_VEVENT_SetMaskAttr(fd, &pHost->sysHost.portChSel[mapChn], stVEVENTMaskCellSize.resol, &stMaskCellAttr) < 0)
			{
				ErrorString("PR1000_VEVENT_SetMaskAttr\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_EVENT_MASKCELL_SIZE:
		{/*{{{*/
			int mapChn = 0;
			_stVEVENTMaskCellSize stVEVENTMaskCellSize;
			_stMaskCellAttr stMaskCellAttr;

			if( (ret = copy_from_user((void *)&stVEVENTMaskCellSize, pArg, sizeof(_stVEVENTMaskCellSize))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_MASKCELL_SIZE(mapChn:%d)\n", stVEVENTMaskCellSize.mapChn);

			mapChn = stVEVENTMaskCellSize.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_GetMaskAttr(fd, &pHost->sysHost.portChSel[mapChn], stVEVENTMaskCellSize.resol, &stMaskCellAttr) < 0)
			{
				ErrorString("PR1000_VEVENT_GetMaskAttr\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			stVEVENTMaskCellSize.cellCntX = stMaskCellAttr.cellCntX;
			stVEVENTMaskCellSize.cellCntY = stMaskCellAttr.cellCntY;
			stVEVENTMaskCellSize.cellWidth = stMaskCellAttr.cellWidth;
			stVEVENTMaskCellSize.cellHeight = stMaskCellAttr.cellHeight;
			stVEVENTMaskCellSize.cellVStartOffset = stMaskCellAttr.cellVStartOffset;
			stVEVENTMaskCellSize.cellHStartOffset = stMaskCellAttr.cellHStartOffset;

			if( (ret = copy_to_user( pArg, &stVEVENTMaskCellSize, sizeof(_stVEVENTMaskCellSize))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_EVENT_MASK:
		{/*{{{*/
			int mapChn = 0;
			_stVEVENTMaskAttr stVEVENTMaskAttr;

			if( (ret = copy_from_user((void *)&stVEVENTMaskAttr, pArg, sizeof(_stVEVENTMaskAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_EVENT_MASK(mapChn:%d)\n", stVEVENTMaskAttr.mapChn);

			mapChn = stVEVENTMaskAttr.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_WriteMaskFormat(fd, &pHost->sysHost.portChSel[mapChn], stVEVENTMaskAttr.maskType, stVEVENTMaskAttr.maskBuf) < 0)
			{
				ErrorString("PR1000_VEVENT_WriteMaskFormat\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_EVENT_MASK:
		{/*{{{*/
			int mapChn = 0;
			_stVEVENTMaskAttr stVEVENTMaskAttr;

			if( (ret = copy_from_user((void *)&stVEVENTMaskAttr, pArg, sizeof(_stVEVENTMaskAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_MASK(mapChn:%d)\n", stVEVENTMaskAttr.mapChn);

			mapChn = stVEVENTMaskAttr.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_ReadMaskFormat(fd, &pHost->sysHost.portChSel[mapChn], stVEVENTMaskAttr.maskType, stVEVENTMaskAttr.maskBuf) < 0)
			{
				ErrorString("PR1000_VEVENT_ReadMaskFormat\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stVEVENTMaskAttr, sizeof(_stVEVENTMaskAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_EVENT_DISPATTR:
		{/*{{{*/
			int mapChn = 0;
			_stVEVENTDisplayAttr stVEVENTDisplayAttr;

			if( (ret = copy_from_user((void *)&stVEVENTDisplayAttr, pArg, sizeof(_stVEVENTDisplayAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_EVENT_DISPATTR(mapChn:%d)\n", stVEVENTDisplayAttr.mapChn);

			mapChn = stVEVENTDisplayAttr.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_SetDisplayCellFormat(fd, &pHost->sysHost.portChSel[mapChn], &stVEVENTDisplayAttr) < 0)
			{
				ErrorString("stVEVENTDisplayAttr\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			memcpy(&pHost->eventHost.stVEVENTDisplayAttr[mapChn], &stVEVENTDisplayAttr, sizeof(_stVEVENTDisplayAttr));
			pHost->eventHost.stVEVENTDisplayAttr[mapChn].mapChn = mapChn;
		}/*}}}*/
		break;
		case PR_IOG_EVENT_DISPATTR:
		{/*{{{*/
			int mapChn = 0;
			_stVEVENTDisplayAttr stVEVENTDisplayAttr;

			if( (ret = copy_from_user((void *)&stVEVENTDisplayAttr, pArg, sizeof(_stVEVENTDisplayAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_DISPATTR(mapChn:%d)\n", stVEVENTDisplayAttr.mapChn);

			mapChn = stVEVENTDisplayAttr.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}
	
			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			pHost->eventHost.stVEVENTDisplayAttr[mapChn].mapChn = mapChn;
			memcpy( &stVEVENTDisplayAttr, &pHost->eventHost.stVEVENTDisplayAttr[mapChn], sizeof(_stVEVENTDisplayAttr));
			if( (ret = copy_to_user( pArg, &stVEVENTDisplayAttr, sizeof(_stVEVENTDisplayAttr))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_EVENT_CLEAR_MASK:
		{/*{{{*/
			uint8_t mapChn = 0;

			if( (ret = copy_from_user((void *)&mapChn, pArg, sizeof(uint8_t))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_EVENT_CLEAR_MASK(mapChn:%d)\n", mapChn);

			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_ClearMask(fd, &pHost->sysHost.portChSel[mapChn]) < 0)
			{
				ErrorString("PR1000_VEVENT_ClearMask\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_EVENT_DETDATA:
		{/*{{{*/
			int mapChn;
			_stVEVENTDetData stVEVENTDetData;

			if( (ret = copy_from_user((void *)&stVEVENTDetData, pArg, sizeof(_stVEVENTDetData))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_DETDATA(mapChn:%d)\n", stVEVENTDetData.mapChn);

			mapChn = stVEVENTDetData.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_GetDetData(fd, &pHost->sysHost.portChSel[mapChn], stVEVENTDetData.startLine, stVEVENTDetData.lineCnt, stVEVENTDetData.detBuf) < 0)
			{
				ErrorString("PR1000_VEVENT_GetDetData\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &stVEVENTDetData, sizeof(_stVEVENTDetData))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

		}/*}}}*/
		break;
#endif // DONT_SUPPORT_EVENT_FUNC
		case PR_IOG_QUERY_WQPOLL:
		{/*{{{*/
			_wqPollChnStatus wqPollChnStatus;
			memcpy(&wqPollChnStatus, &pHost->wqPollChnStatus, sizeof(_wqPollChnStatus));
			memset(&pHost->wqPollChnStatus, 0, sizeof(_wqPollChnStatus));

			DbgString("PR_IOG_QUERY_WQPOLL\n");

			if( (ret = copy_to_user( pArg, &wqPollChnStatus, sizeof(_wqPollChnStatus))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOG_EVENT_DET_STD:
		{/*{{{*/
			int mapChn;
			_stEventDetStd stEventDetStd;

			if( (ret = copy_from_user((void *)&stEventDetStd, pArg, sizeof(_stEventDetStd))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_DET_STD(mapChn:%d)\n", stEventDetStd.mapChn);

			mapChn = stEventDetStd.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			pHost->eventHost.stEventDetStd[mapChn].mapChn = mapChn;
			memcpy(&stEventDetStd, &pHost->eventHost.stEventDetStd[mapChn], sizeof(_stEventDetStd));

			if( (ret = copy_to_user( pArg, &stEventDetStd, sizeof(_stEventDetStd))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOG_EVENT_NOVID_STATUS:
		{/*{{{*/
			uint8_t status = 0;

			if( (ret = copy_from_user((void *)&status, pArg, sizeof(uint8_t))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_NOVID_STATUS(mapChn:%d)\n", status);

			if( (status >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[status].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", status);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_GetNovidStatus(fd, &pHost->sysHost.portChSel[status], &status) < 0)
			{
				ErrorString("PR1000_VEVENT_GetNovidStatus\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &status, sizeof(uint8_t))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

		}/*}}}*/
		break;
		case PR_IOG_EVENT_VFD_STATUS:
		{/*{{{*/
			uint8_t status = 0;

			if( (ret = copy_from_user((void *)&status, pArg, sizeof(uint8_t))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_VFD_STATUS(mapChn:%d)\n", status);

			if( (status >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[status].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", status);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_GetVfdStatus(fd, &pHost->sysHost.portChSel[status], &status) < 0)
			{
				ErrorString("PR1000_VEVENT_GetVfdStatus\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &status, sizeof(uint8_t))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

		}/*}}}*/
		break;
#ifndef DONT_SUPPORT_EVENT_FUNC
		case PR_IOG_EVENT_MD_STATUS:
		{/*{{{*/
			uint8_t status = 0;

			if( (ret = copy_from_user((void *)&status, pArg, sizeof(uint8_t))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Print("PR_IOG_EVENT_MD_STATUS(mapChn:%d)\n", status);

			if( (status >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[status].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", status);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_GetMdStatus(fd, &pHost->sysHost.portChSel[status], &status) < 0)
			{
				ErrorString("PR1000_VEVENT_GetMdStatus\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &status, sizeof(uint8_t))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

		}/*}}}*/
		break;
		case PR_IOG_EVENT_BD_STATUS:
		{/*{{{*/
			uint8_t status = 0;

			if( (ret = copy_from_user((void *)&status, pArg, sizeof(uint8_t))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_BD_STATUS(mapChn:%d)\n", status);

			if( (status >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[status].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", status);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_GetBdStatus(fd, &pHost->sysHost.portChSel[status], &status) < 0)
			{
				ErrorString("PR1000_VEVENT_GetBdStatus\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &status, sizeof(uint8_t))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

		}/*}}}*/
		break;
		case PR_IOG_EVENT_ND_STATUS:
		{/*{{{*/
			uint8_t status = 0;

			if( (ret = copy_from_user((void *)&status, pArg, sizeof(uint8_t))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_ND_STATUS(mapChn:%d)\n", status);

			if( (status >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[status].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", status);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_GetNdStatus(fd, &pHost->sysHost.portChSel[status], &status) < 0)
			{
				ErrorString("PR1000_VEVENT_GetNdStatus\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &status, sizeof(uint8_t))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

		}/*}}}*/
		break;
		case PR_IOG_EVENT_DD_STATUS:
		{/*{{{*/
			uint8_t status = 0;

			if( (ret = copy_from_user((void *)&status, pArg, sizeof(uint8_t))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_DD_STATUS(mapChn:%d)\n", status);

			if( (status >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[status].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", status);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_GetDdStatus(fd, &pHost->sysHost.portChSel[status], &status) < 0)
			{
				ErrorString("PR1000_VEVENT_GetDdStatus\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &status, sizeof(uint8_t))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

		}/*}}}*/
		break;
		case PR_IOG_EVENT_DFD_STATUS:
		{/*{{{*/
			uint8_t status = 0;

			if( (ret = copy_from_user((void *)&status, pArg, sizeof(uint8_t))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_DFD_STATUS(mapChn:%d)\n", status);

			if( (status >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[status].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", status);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_VEVENT_GetDfdStatus(fd, &pHost->sysHost.portChSel[status], &status) < 0)
			{
				ErrorString("PR1000_VEVENT_GetDfdStatus\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &status, sizeof(uint8_t))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

		}/*}}}*/
		break;
		case PR_IOG_EVENT_ADMUTE_STATUS:
		{/*{{{*/
			uint8_t prChip = 0;
			uint8_t prChn = 0;
			uint8_t status = 0;

			if( (ret = copy_from_user((void *)&prChn, pArg, sizeof(uint8_t))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_EVENT_ADMUTE_STATUS(prChn:%d)\n", prChn);

			if( (prChn >= (MAX_PR1000_CHIP*DEF_PR1000_MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}
			prChip = prChn / DEF_PR1000_MAX_CHN;
			prChn = prChn % DEF_PR1000_MAX_CHN;

			if( PR1000_VEVENT_GetAdMuteStatus(fd, prChip, prChn, &status) < 0)
			{
				ErrorString("PR1000_VEVENT_GetAdMuteStatus\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}

			if( (ret = copy_to_user( pArg, &status, sizeof(uint8_t))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

		}/*}}}*/
		break;
		case PR_IOG_EVENT_ADABS_STATUS:
		{/*{{{*/
			int chip, ch = 0;
			uint8_t status = 0;
			uint32_t bStatus = 0;

			for(chip = 0; chip < PR1000_CHIP_COUNT; chip++)
			{
				for(ch = 0; ch < DEF_PR1000_MAX_CHN; ch++)
				{
					PR1000_VEVENT_GetAdAbsStatus(fd, chip, ch, &status);
					bStatus |= (status & 1) << ((chip*DEF_PR1000_MAX_CHN)+ch);
				}
			}

			if( (ret = copy_to_user( pArg, &bStatus, sizeof(uint32_t))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

		}/*}}}*/
		break;
		case PR_IOG_EVENT_ADDIFF_STATUS:
		{/*{{{*/
			int chip, ch = 0;
			uint8_t status = 0;
			uint32_t bStatus = 0;

			for(chip = 0; chip < PR1000_CHIP_COUNT; chip++)
			{
				for(ch = 0; ch < DEF_PR1000_MAX_CHN; ch++)
				{
					PR1000_VEVENT_GetAdDiffStatus(fd, chip, ch, &status);
					bStatus |= (status & 1) << ((chip*DEF_PR1000_MAX_CHN)+ch);
				}
			}

			if( (ret = copy_to_user( pArg, &bStatus, sizeof(uint32_t))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

		}/*}}}*/
		break;
#endif // DONT_SUPPORT_EVENT_FUNC
#ifndef DONT_SUPPORT_PTZ_FUNC
		case PR_IOS_PTZ_INIT:
		{/*{{{*/
			uint8_t mapChn = 0;

			if( (ret = copy_from_user((void *)&mapChn, pArg, sizeof(uint8_t))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_PTZ_INIT(mapChn:%d)\n", mapChn);

			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_PTZ_Init(fd, &pHost->sysHost.portChSel[mapChn], pHost) < 0)
			{
				ErrorString("PR1000_PTZ_Init\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
#ifndef DONT_SUPPORT_PTZ_FUNC
		case PR_IOS_PTZ_TXPARAM:
		{/*{{{*/
			int mapChn;
			_stPTZTxParam stPtzTxParam;

			if( (ret = copy_from_user((void *)&stPtzTxParam, pArg, sizeof(_stPTZTxParam))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Print("PR_IOS_PTZ_TXPARAM(mapChn:%d)\n", stPtzTxParam.mapChn);

			mapChn = stPtzTxParam.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_PTZ_SetTxParam(fd, &pHost->sysHost.portChSel[mapChn], &stPtzTxParam) < 0)
			{
				ErrorString("PR1000_PTZ_SetTxParam\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_PTZ_TXPARAM:
		{/*{{{*/
			int mapChn;
			_stPTZTxParam stPtzTxParam;

			if( (ret = copy_from_user((void *)&stPtzTxParam, pArg, sizeof(_stPTZTxParam))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Print("PR_IOG_PTZ_TXPARAM(mapChn:%d)\n", stPtzTxParam.mapChn);

			mapChn = stPtzTxParam.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_PTZ_GetTxParam(fd, &pHost->sysHost.portChSel[mapChn], &stPtzTxParam) < 0)
			{
				ErrorString("PR1000_PTZ_GetTxParam\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
#endif // DONT_SUPPORT_PTZ_FUNC
		case PR_IOS_PTZ_RXPARAM:
		{/*{{{*/
			int mapChn;
			_stPTZRxParam stPtzRxParam;

			if( (ret = copy_from_user((void *)&stPtzRxParam, pArg, sizeof(_stPTZRxParam))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Print("PR_IOS_PTZ_RXPARAM(mapChn:%d)\n", stPtzRxParam.mapChn);

			mapChn = stPtzRxParam.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_PTZ_SetRxParam(fd, &pHost->sysHost.portChSel[mapChn], &stPtzRxParam) < 0)
			{
				ErrorString("PR1000_PTZ_SetRxParam\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOG_PTZ_RXPARAM:
		{/*{{{*/
			int mapChn;
			_stPTZRxParam stPtzRxParam;

			if( (ret = copy_from_user((void *)&stPtzRxParam, pArg, sizeof(_stPTZRxParam))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Print("PR_IOG_PTZ_RXPARAM(mapChn:%d)\n", stPtzRxParam.mapChn);

			mapChn = stPtzRxParam.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_PTZ_GetRxParam(fd, &pHost->sysHost.portChSel[mapChn], &stPtzRxParam) < 0)
			{
				ErrorString("PR1000_PTZ_GetRxParam\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOS_PTZ_TX_PAT_INFO:
		{/*{{{*/
			int mapChn;
			_stPTZTxPatInfo stPTZTxPatInfo;

			_stPortChSel *pstPortChSel = NULL;
			uint8_t prChip, prChn, i2cSlaveAddr; 


			if( (ret = copy_from_user((void *)&stPTZTxPatInfo, pArg, sizeof(_stPTZTxPatInfo))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_PTZ_TX_PAT_FORMAT(mapChn:%d)\n", stPTZTxPatInfo.mapChn);

			mapChn = stPTZTxPatInfo.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			pstPortChSel = &pHost->sysHost.portChSel[mapChn];
			prChip = pstPortChSel->prChip;
			prChn = pstPortChSel->prChn;
			i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

			switch(pHost->sysHost.gPR1000RxType[mapChn])
			{/*{{{*/
				case pr1000_format_SD720:
					{/*{{{*/
						if( PR1000_PTZ_WriteTxPattern(fd, i2cSlaveAddr, prChn, (const char *)stPTZTxPatInfo.pPatFormat, stPTZTxPatInfo.sizePatFormat, (const char *)stPTZTxPatInfo.pPatData, stPTZTxPatInfo.sizePatData) < 0)
						{
							ErrorString("PR1000_PTZ_SD720_WriteTxPattern\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
						}
					}/*}}}*/
					break;
				case pr1000_format_SD960:
					{/*{{{*/
						if( PR1000_PTZ_WriteTxPattern(fd, i2cSlaveAddr, prChn, (const char *)stPTZTxPatInfo.pPatFormat, stPTZTxPatInfo.sizePatFormat, (const char *)stPTZTxPatInfo.pPatData, stPTZTxPatInfo.sizePatData) < 0)
						{
							ErrorString("PR1000_PTZ_SD960_WriteTxPattern\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
						}
					}/*}}}*/
					break;
#ifndef DONT_SUPPORT_STD_PVI
				case pr1000_format_PVI:
					{/*{{{*/
						if( PR1000_PTZ_WriteTxPattern(fd, i2cSlaveAddr, prChn, (const char *)stPTZTxPatInfo.pPatFormat, stPTZTxPatInfo.sizePatFormat, (const char *)stPTZTxPatInfo.pPatData, stPTZTxPatInfo.sizePatData) < 0)
						{
							ErrorString("PR1000_PTZ_WriteTxPattern\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
						}
					}/*}}}*/
					break;
#endif // DONT_SUPPORT_STD_PVI
				case pr1000_format_HDA:
					{/*{{{*/
						if( PR1000_PTZ_WriteTxPattern(fd, i2cSlaveAddr, prChn, (const char *)stPTZTxPatInfo.pPatFormat, stPTZTxPatInfo.sizePatFormat, (const char *)stPTZTxPatInfo.pPatData, stPTZTxPatInfo.sizePatData) < 0)
						{
							ErrorString("PR1000_PTZ_HDA_WriteTxPattern\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
						}
					}/*}}}*/
					break;
				case pr1000_format_CVI:
					{/*{{{*/
						if( PR1000_PTZ_WriteTxPattern(fd, i2cSlaveAddr, prChn, (const char *)stPTZTxPatInfo.pPatFormat, stPTZTxPatInfo.sizePatFormat, (const char *)stPTZTxPatInfo.pPatData, stPTZTxPatInfo.sizePatData) < 0)
						{
							ErrorString("PR1000_PTZ_CVI_WriteTxPattern\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
						}
					}/*}}}*/
					break;
				case pr1000_format_HDT:
					{/*{{{*/
						if( PR1000_PTZ_WriteTxPattern(fd, i2cSlaveAddr, prChn, (const char *)stPTZTxPatInfo.pPatFormat, stPTZTxPatInfo.sizePatFormat, (const char *)stPTZTxPatInfo.pPatData, stPTZTxPatInfo.sizePatData) < 0)
						{
							ErrorString("PR1000_PTZ_HDT_WriteTxPattern\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
						}
					}/*}}}*/
					break;
				case pr1000_format_HDT_NEW:
					{/*{{{*/
						if( PR1000_PTZ_WriteTxPattern(fd, i2cSlaveAddr, prChn, (const char *)stPTZTxPatInfo.pPatFormat, stPTZTxPatInfo.sizePatFormat, (const char *)stPTZTxPatInfo.pPatData, stPTZTxPatInfo.sizePatData) < 0)
						{
							ErrorString("PR1000_PTZ_HDT_NEW_WriteTxPattern\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
						}
					}/*}}}*/
					break;
				default: 
					{/*{{{*/
						if( PR1000_PTZ_WriteTxPattern(fd, i2cSlaveAddr, prChn, (const char *)stPTZTxPatInfo.pPatFormat, stPTZTxPatInfo.sizePatFormat, (const char *)stPTZTxPatInfo.pPatData, stPTZTxPatInfo.sizePatData) < 0)
						{
							ErrorString("PR1000_PTZ_WriteTxPattern\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
						}
					}/*}}}*/
					break;
			}/*}}}*/
		}/*}}}*/
		break;
		case PR_IOS_PTZ_RX_PAT_INFO:
		{/*{{{*/
			int mapChn;
			_stPTZRxPatInfo stPTZRxPatInfo;

			_stPortChSel *pstPortChSel = NULL;
			uint8_t prChip, prChn, i2cSlaveAddr; 
			uint8_t i2cReg, i2cData;
			uint8_t i2cMask; 


			if( (ret = copy_from_user((void *)&stPTZRxPatInfo, pArg, sizeof(_stPTZRxPatInfo))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_PTZ_RX_PAT_INFO(mapChn:%d)\n", stPTZRxPatInfo.mapChn);

			mapChn = stPTZRxPatInfo.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			pstPortChSel = &pHost->sysHost.portChSel[mapChn];
			prChip = pstPortChSel->prChip;
			prChn = pstPortChSel->prChn;
			i2cSlaveAddr = pstPortChSel->i2cSlvAddr;

			switch(pHost->sysHost.gPR1000RxType[mapChn])
			{/*{{{*/
				case pr1000_format_SD720:
				case pr1000_format_SD960:
					{/*{{{*/
						if( PR1000_PTZ_WriteRxPattern(fd, i2cSlaveAddr, prChn, (const char *)stPTZRxPatInfo.pPatFormat, stPTZRxPatInfo.sizePatFormat, (const char *)stPTZRxPatInfo.pPatStartFormat, stPTZRxPatInfo.sizePatStartFormat, (const char *)stPTZRxPatInfo.pPatStartData, stPTZRxPatInfo.sizePatStartData) < 0)
						{
							ErrorString("PR1000_PTZ_SD_WriteRxPattern\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
						}
						/* set rd data mode for receive data. */
						i2cMask = 0xD7; i2cData = 0x93|((PR1000_PTZ_SD_RX_DATA_BITSWAP&0x1)<<6);
						i2cReg = PR1000_REG_ADDR_PTZ_FIFO_RD_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
						if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
						}
					}/*}}}*/
					break;
#ifndef DONT_SUPPORT_STD_PVI
				case pr1000_format_PVI:
					{/*{{{*/
						if( PR1000_PTZ_WriteRxPattern(fd, i2cSlaveAddr, prChn, (const char *)stPTZRxPatInfo.pPatFormat, stPTZRxPatInfo.sizePatFormat, (const char *)stPTZRxPatInfo.pPatStartFormat, stPTZRxPatInfo.sizePatStartFormat, (const char *)stPTZRxPatInfo.pPatStartData, stPTZRxPatInfo.sizePatStartData) < 0)
						{
							ErrorString("PR1000_PTZ_WriteRxPattern\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
						}
						/* set rd data mode for receive data. */
						i2cMask = 0xD7; i2cData = 0x93|((PR1000_PTZ_PVI_RX_DATA_BITSWAP&0x1)<<6);
						i2cReg = PR1000_REG_ADDR_PTZ_FIFO_RD_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
						if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
						}
					}/*}}}*/
					break;
#endif // DONT_SUPPORT_STD_PVI
				case pr1000_format_HDA:
					{/*{{{*/
						if( PR1000_PTZ_WriteRxPattern(fd, i2cSlaveAddr, prChn, (const char *)stPTZRxPatInfo.pPatFormat, stPTZRxPatInfo.sizePatFormat, (const char *)stPTZRxPatInfo.pPatStartFormat, stPTZRxPatInfo.sizePatStartFormat, (const char *)stPTZRxPatInfo.pPatStartData, stPTZRxPatInfo.sizePatStartData) < 0)
						{
							ErrorString("PR1000_PTZ_HDA_WriteRxPattern\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
						}
						/* set rd data mode for receive data. */
						i2cMask = 0xD7; i2cData = 0x93|((PR1000_PTZ_HDA_RX_DATA_BITSWAP&0x1)<<6);
						i2cReg = PR1000_REG_ADDR_PTZ_FIFO_RD_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
						if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
						}
					}/*}}}*/
					break;
				case pr1000_format_CVI:
					{/*{{{*/
						if( PR1000_PTZ_WriteRxPattern(fd, i2cSlaveAddr, prChn, (const char *)stPTZRxPatInfo.pPatFormat, stPTZRxPatInfo.sizePatFormat, (const char *)stPTZRxPatInfo.pPatStartFormat, stPTZRxPatInfo.sizePatStartFormat, (const char *)stPTZRxPatInfo.pPatStartData, stPTZRxPatInfo.sizePatStartData) < 0)
						{
							ErrorString("PR1000_PTZ_CVI_WriteRxPattern\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
						}
						/* set rd data mode for receive data. */
						i2cMask = 0xD7; i2cData = 0x93|((PR1000_PTZ_CVI_RX_DATA_BITSWAP&0x1)<<6);
						i2cReg = PR1000_REG_ADDR_PTZ_FIFO_RD_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
						if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
						}
					}/*}}}*/
					break;
				case pr1000_format_HDT:
					{/*{{{*/
						if( PR1000_PTZ_WriteRxPattern(fd, i2cSlaveAddr, prChn, (const char *)stPTZRxPatInfo.pPatFormat, stPTZRxPatInfo.sizePatFormat, (const char *)stPTZRxPatInfo.pPatStartFormat, stPTZRxPatInfo.sizePatStartFormat, (const char *)stPTZRxPatInfo.pPatStartData, stPTZRxPatInfo.sizePatStartData) < 0)
						{
							ErrorString("PR1000_PTZ_HDT_WriteRxPattern\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
						}
						/* set rd data mode for receive data. */
						i2cMask = 0xD7; i2cData = 0x93|((PR1000_PTZ_HDT_RX_DATA_BITSWAP&0x1)<<6);
						i2cReg = PR1000_REG_ADDR_PTZ_FIFO_RD_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
						if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
						}
					}/*}}}*/
					break;
				case pr1000_format_HDT_NEW:
					{/*{{{*/
						if( PR1000_PTZ_WriteRxPattern(fd, i2cSlaveAddr, prChn, (const char *)stPTZRxPatInfo.pPatFormat, stPTZRxPatInfo.sizePatFormat, (const char *)stPTZRxPatInfo.pPatStartFormat, stPTZRxPatInfo.sizePatStartFormat, (const char *)stPTZRxPatInfo.pPatStartData, stPTZRxPatInfo.sizePatStartData) < 0)
						{
							ErrorString("PR1000_PTZ_HDT_NEW_WriteRxPattern\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
						}
						/* set rd data mode for receive data. */
						i2cMask = 0xD7; i2cData = 0x93|((PR1000_PTZ_HDT_NEW_RX_DATA_BITSWAP&0x1)<<6);
						i2cReg = PR1000_REG_ADDR_PTZ_FIFO_RD_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
						if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
						}
					}/*}}}*/
					break;
				default: 
					{/*{{{*/
						if( PR1000_PTZ_WriteRxPattern(fd, i2cSlaveAddr, prChn, (const char *)stPTZRxPatInfo.pPatFormat, stPTZRxPatInfo.sizePatFormat, (const char *)stPTZRxPatInfo.pPatStartFormat, stPTZRxPatInfo.sizePatStartFormat, (const char *)stPTZRxPatInfo.pPatStartData, stPTZRxPatInfo.sizePatStartData) < 0)
						{
							ErrorString("PR1000_PTZ_WriteRxPattern\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
						}
						/* set rd data mode for receive data. */
						i2cMask = 0xD7; i2cData = 0x93|((PR1000_PTZ_CVI_RX_DATA_BITSWAP&0x1)<<6);
						i2cReg = PR1000_REG_ADDR_PTZ_FIFO_RD_INIT + PR1000_OFFSETADDR_PTZ_CH(prChn);
						if( (PR1000_WriteMaskBit(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cMask, i2cData)) < 0)
						{
							ErrorString("Write reg.\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
						}
					}/*}}}*/
					break;
			}/*}}}*/
		}/*}}}*/
		break;
		case PR_IOS_PTZ_START_RX:
		{/*{{{*/
			int mapChn;
			_stPtzStart stPtzStart;

			if( (ret = copy_from_user((void *)&stPtzStart, pArg, sizeof(_stPtzStart))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_PTZ_START_RX(mapChn:%d)\n", stPtzStart.mapChn);

			mapChn = stPtzStart.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_PTZ_StartRX(fd, &pHost->sysHost.portChSel[mapChn], stPtzStart.bStart) < 0)
			{
				ErrorString("PR1000_PTZ_StartRX\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOS_PTZ_START_TX:
		{/*{{{*/
			int mapChn;
			_stPtzStart stPtzStart;

			if( (ret = copy_from_user((void *)&stPtzStart, pArg, sizeof(_stPtzStart))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_PTZ_START_TX(mapChn:%d)\n", stPtzStart.mapChn);

			mapChn = stPtzStart.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( PR1000_PTZ_StartTX(fd, &pHost->sysHost.portChSel[mapChn], stPtzStart.bStart) < 0)
			{
				ErrorString("PR1000_PTZ_StartTX\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}
		}/*}}}*/
		break;
		case PR_IOS_PTZ_SEND_TXDATA:
		{/*{{{*/
			int mapChn;
			int waitCnt = 0;
			uint8_t camType, camResol;
			uint8_t *pPtzCmd;
			uint16_t ptzCmdLength;
			_stPtzSend stPtzSend;

			if( (ret = copy_from_user((void *)&stPtzSend, pArg, sizeof(_stPtzSend))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_PTZ, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_PTZ_SEND_TXDATA(mapChn:%d)\n", stPtzSend.mapChn);

			mapChn = stPtzSend.mapChn;
			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_PTZ, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			camType = stPtzSend.camType;
			camResol = stPtzSend.camResol;
			pPtzCmd = stPtzSend.ptzCmd;
			ptzCmdLength = stPtzSend.ptzCmdLength;

			/* wait send tx status. */
			waitCnt = 10;
			while( (PR1000_PTZ_CheckSendTxStatus(fd, &pHost->sysHost.portChSel[mapChn]) < 0) && waitCnt-- )
			{
				DbgString("Wait send tx status\n");
				mdelay(30);
			}
			if(waitCnt <= 0)
			{
				ErrorString("TimeOver send tx status.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_PTZ, PR_ERR_BUSY));
			}
			switch(camType)
			{/*{{{*/
				case pr1000_format_SD720:
				case pr1000_format_SD960:
					{
						if( (PR1000_PTZ_SD_SendTxData(fd, &pHost->sysHost.portChSel[mapChn], camResol, pPtzCmd, ptzCmdLength)) < 0 )
						{
							ErrorString("Can't send tx data\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_PTZ, PR_ERR_NOT_PERM));
						}
					}
					break;
#ifndef DONT_SUPPORT_STD_PVI
				case pr1000_format_PVI:
					{
						if( (PR1000_PTZ_PVI_SendTxData(fd, &pHost->sysHost.portChSel[mapChn], camResol, pPtzCmd, ptzCmdLength)) < 0 )
						{
							ErrorString("Can't send tx data\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_PTZ, PR_ERR_NOT_PERM));
						}
					}
					break;
#endif // DONT_SUPPORT_STD_PVI
				case pr1000_format_HDA:
					{
						if( (PR1000_PTZ_HDA_SendTxData(fd, &pHost->sysHost.portChSel[mapChn], camResol, pPtzCmd, ptzCmdLength)) < 0 )
						{
							ErrorString("Can't send tx data\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_PTZ, PR_ERR_NOT_PERM));
						}
					}
					break;
				case pr1000_format_CVI:
					{
						if( (PR1000_PTZ_CVI_SendTxData(fd, &pHost->sysHost.portChSel[mapChn], camResol, pPtzCmd, ptzCmdLength)) < 0 )
						{
							ErrorString("Can't send tx data\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_PTZ, PR_ERR_NOT_PERM));
						}
					}
					break;
				case pr1000_format_HDT:
					{
						if( (PR1000_PTZ_HDT_SendTxData(fd, &pHost->sysHost.portChSel[mapChn], camResol, pPtzCmd, ptzCmdLength)) < 0 )
						{
							ErrorString("Can't send tx data\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_PTZ, PR_ERR_NOT_PERM));
						}
					}
					break;
				case pr1000_format_HDT_NEW:
					{
						if( (PR1000_PTZ_HDT_NEW_SendTxData(fd, &pHost->sysHost.portChSel[mapChn], camResol, pPtzCmd, ptzCmdLength)) < 0 )
						{
							ErrorString("Can't send tx data\n");
							return(PR_ERROR_CODE_DRV(PR_ERR_PTZ, PR_ERR_NOT_PERM));
						}
					}
					break;
				default:
					{
						Error("invalid ptz type(%d-%d)\n", camType COMMA camResol);
						return(PR_ERROR_CODE_DRV(PR_ERR_PTZ, PR_ERR_NOT_SUPPORT));
					}
					break;
			}/*}}}*/

			pHost->ptzHost.u32PtzTxCnt[mapChn]++;

		}/*}}}*/
		break;
		case PR_IOG_PTZ_RECV_SIZE:
		{/*{{{*/
			uint8_t mapChn;
			int size;
        		FIFO *pPtzRcvQ = NULL;
		        SPINLOCK_T *pLockPtzRcvQ = NULL;

			if( (ret = copy_from_user((void *)&mapChn, pArg, sizeof(uint8_t))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_PTZ, PR_ERR_NOT_PERM));
			}
			Dbg("PR_IOG_PTZ_RECV_SIZE(mapChn:%d)\n", mapChn);

			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_PTZ, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

        		pPtzRcvQ = (FIFO *)&pHost->ptzHost.ptzRecvQ[mapChn];
		        pLockPtzRcvQ = (SPINLOCK_T *)&pHost->ptzHost.splockPtzRecvQ[mapChn];

			size = PR1000_CQ_howmany(pPtzRcvQ, pLockPtzRcvQ);

			if( (ret = copy_to_user( pArg, &size, sizeof(size))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

		}/*}}}*/
		break;
		case PR_IOG_PTZ_RECV_RXDATA:
		{/*{{{*/
			_stPtzRecv stPtzRecv;
			int mapChn;
			int size;
        		FIFO *pPtzRcvQ = NULL;
		        SPINLOCK_T *pLockPtzRcvQ = NULL;
			int i, rxCnt;
        		char *pPtzRcvData = NULL;
			char *pRxBuf;

			if( (ret = copy_from_user((void *)&stPtzRecv, pArg, sizeof(_stPtzRecv))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_PTZ, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOG_PTZ_RECV_RXDATA(mapChn:%d)\n", stPtzRecv.mapChn);

			mapChn = stPtzRecv.mapChn;
			size = stPtzRecv.size;
			pRxBuf = stPtzRecv.pPtzRxBuf;

			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			if( (size <= 0) || (stPtzRecv.pPtzRxBuf == NULL) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

        		pPtzRcvQ = (FIFO *)&pHost->ptzHost.ptzRecvQ[mapChn];
		        pLockPtzRcvQ = (SPINLOCK_T *)&pHost->ptzHost.splockPtzRecvQ[mapChn];
			rxCnt = 0;
			for(i = 0; i < size; i++)
			{
				if(PR1000_CQ_is_empty(pPtzRcvQ))
				{
					break;
				}

				if( (pPtzRcvData = (char *)PR1000_CQ_get_locked(pPtzRcvQ, pLockPtzRcvQ)) == NULL )
				{
					break;
				}
				pRxBuf[i] = *pPtzRcvData;
				PR1000_CQ_free_item(pPtzRcvData);
				rxCnt++;
			}

			if( (ret = copy_to_user( pArg, (const void *)&stPtzRecv, sizeof(_stPtzRecv))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;
		case PR_IOS_PTZ_HDT_CHGOLD_CMD:
		{/*{{{*/
			uint8_t mapChn;
			uint8_t prChip, prChn, i2cSlaveAddr; 
			uint8_t i2cReg, i2cData;

			if( (ret = copy_from_user((void *)&mapChn, pArg, sizeof(uint8_t))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_PTZ, PR_ERR_NOT_PERM));
			}

			Dbg("PR_IOS_PTZ_HDT_CHGOLD_CMD(mapChn:%d)\n", mapChn);

			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			mapChn = pHost->sysHost.portChSel[mapChn].chn;
			prChip = pHost->sysHost.portChSel[mapChn].prChip;
			prChn = pHost->sysHost.portChSel[mapChn].prChn;
			i2cSlaveAddr = pHost->sysHost.portChSel[mapChn].i2cSlvAddr;

			PR1000_PTZ_WriteTxPattern(fd, i2cSlaveAddr, prChn, pr1000_ptz_table_hdt_chgold_tx_pat_format, sizeof(pr1000_ptz_table_hdt_chgold_tx_pat_format), pr1000_ptz_table_hdt_chgold_tx_pat_data, sizeof(pr1000_ptz_table_hdt_chgold_tx_pat_data));
			/* CHGOLD Parameter */
			{/*{{{*/
				i2cData = 0x04;
				i2cReg = 0x22 + PR1000_OFFSETADDR_PTZ_CH(prChn);
				if( (PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
				i2cData = 0x02;
				i2cReg = 0x23 + PR1000_OFFSETADDR_PTZ_CH(prChn);
				if( (PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
				i2cData = 0x70;
				i2cReg = 0x24 + PR1000_OFFSETADDR_PTZ_CH(prChn);
				if( (PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
				i2cData = 0x64;
				i2cReg = 0x25 + PR1000_OFFSETADDR_PTZ_CH(prChn);
				if( (PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
				i2cData = 0x02;
				i2cReg = 0x26 + PR1000_OFFSETADDR_PTZ_CH(prChn);
				if( (PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
				i2cData = 0x70;
				i2cReg = 0x27 + PR1000_OFFSETADDR_PTZ_CH(prChn);
				if( (PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
				i2cData = 0x64;
				i2cReg = 0x28 + PR1000_OFFSETADDR_PTZ_CH(prChn);
				if( (PR1000_PageWrite(fd, i2cSlaveAddr, PR1000_REG_PAGE_PTZ, i2cReg, i2cData)) < 0)
				{
					ErrorString("Write reg.\n");
					return(-1);
				}
			}/*}}}*/

			PR1000_PTZ_HDT_CHGOLD_SendTxCmd(fd, &pHost->sysHost.portChSel[mapChn], pHost->sysHost.gPR1000RxResol[mapChn]);
		}/*}}}*/
		break;
#endif // DONT_SUPPORT_PTZ_FUNC
		case PR_IOG_DET_CEQINFO:
		{/*{{{*/
			uint8_t mapChn;
			_stCEQInfo stCEQInfo;

			if( (ret = copy_from_user((void *)&stCEQInfo, pArg, sizeof(_stCEQInfo))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}

			mapChn = stCEQInfo.mapChn;
			Dbg("PR_IOG_DET_CEQINFO(mapChn:%d)\n", mapChn);

			if( (mapChn >= (MAX_CHN)) )
			{
				ErrorString("Invalid argu.\n");
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_INVALID_CHNID));
			}

			if(!ASSERT_VALID_CH(pHost->sysHost.portChSel[mapChn].i2cSlvAddr)) 
			{
				Error("Not config(mapChn:%d)\n", mapChn);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_CONFIG));
			}

			PR1000_CEQ_GetEQInfo(fd, &pHost->sysHost.portChSel[mapChn], &stCEQInfo);
			if( (ret = copy_to_user( pArg, &stCEQInfo, sizeof(_stCEQInfo))) < 0)
			{
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_NOT_PERM));
			}
		}/*}}}*/
		break;

		default:
			{
				Error("IOCTL: invalid ioctl cmd[0x%08x]\n", cmd);
				return(PR_ERROR_CODE_DRV(PR_ERR_COMMON, PR_ERR_ILLEGAL_PARAM));
			}		    
	}

	return ret;
}
#endif // __LINUX_SYSTEM__

#ifdef __LINUX_SYSTEM__
static struct file_operations pr1000_fops = {
	.owner      = THIS_MODULE,
	//.ioctl      = pr1000_ioctl,
	.unlocked_ioctl  = pr1000_ioctl,
	.open       = pr1000_open,
        .poll       = pr1000_poll,
	.release    = pr1000_close
};

#ifdef CONFIG_HISI_SNAPSHOT_BOOT
struct himedia_ops himedia_pr1000DrvOps =
{
    .pm_freeze = pr1000_freeze,
    .pm_restore  = pr1000_restore
};
#else
static struct miscdevice pr1000_dev = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= "pr1000_dev",
	.fops  		= &pr1000_fops,
};
#endif // CONFIG_HISI_SNAPSHOT_BOOT
#endif // __LINUX_SYSTEM__


/* ################################################################### */
/*! interrupt routine                                                          */
/* ################################################################### */
#ifdef __LINUX_SYSTEM__
irqreturn_t IrqHandler(int irq, void *pDevInstance)
{
        _drvHost *pHost = pDevInstance;
        unsigned long irqFlag;

        SPIN_LOCK_IRQSAVE(&pHost->spLockIrq, irqFlag);

        DbgString("entry irq handler \n");
	PR1000_IRQ_Isr(-1, pHost); //Should set fd = -1. -> In isr call.

        goto IrqHandler_ret;

IrqHandler_ret:
        SPIN_UNLOCK_IRQRESTORE(&pHost->spLockIrq, irqFlag);

        return(IRQ_HANDLED);
}
#else //#ifdef __LINUX_SYSTEM__
int IrqHandler(void)
{
        _drvHost *pHost = gpDrvHost;

	if(pHost == NULL) return(0);

        SPIN_LOCK_IRQSAVE(&pHost->spLockIrq, NULL);

        DbgString("entry irq handler \n");
	PR1000_IRQ_Isr(-1, pHost); //Should set fd = -1. -> In isr call.

        goto IrqHandler_ret;

IrqHandler_ret:
        SPIN_UNLOCK_IRQRESTORE(&pHost->spLockIrq, NULL);

        return(0);
}
#endif // __LINUX_SYSTEM__

#ifdef __LINUX_SYSTEM__
static int __init pr1000_module_init(void)
{
	int ret = 0, i = 0;

	if( (gpDrvHost = (_drvHost *)ZALLOC(sizeof(_drvHost), GFP_KERNEL)) == NULL)
	{
		ret = -ENOMEM;
		printk("ERROR: Can't allocate memory\n");
		return(ret);
	}
	SPIN_LOCK_INIT(&gpDrvHost->spLockIrq);
        init_waitqueue_head(&gpDrvHost->wqPoll);
        memset(&gpDrvHost->wqPollChnStatus, 0, sizeof(_wqPollChnStatus));

#ifdef CONFIG_HISI_SNAPSHOT_BOOT
	snprintf(himedia_pr1000Device.devfs_name, sizeof(himedia_pr1000Device.devfs_name), DEV_NAME);
	himedia_pr1000Device.minor  = HIMEDIA_DYNAMIC_MINOR;
	himedia_pr1000Device.fops   = &pr1000_fops;
	himedia_pr1000Device.drvops = &himedia_pr1000DrvOps;
	himedia_pr1000Device.owner  = THIS_MODULE;

	ret = himedia_register(&himedia_pr1000Device);
	if (ret)
	{
		printk("ERROR: could not register himedia pr1000 device");

		if(gpDrvHost)
		{
			FREE(gpDrvHost);
		}
		return ret;
	}
#else
	/* register misc device*/
	ret = misc_register(&pr1000_dev);
	if (ret)
	{
		printk("ERROR: could not register pr1000 devices\n");

		if(gpDrvHost)
		{
			FREE(gpDrvHost);
		}
		return ret;
	}
#endif // CONFIG_HISI_SNAPSHOT_BOOT

	/* create proc fs */
#ifdef SUPPORT_PROC_SYSTEM //support proc system for linux debugging.
	if( gpPr1000RootProcDir == NULL)
	{/*{{{*/
		//printk("mkdir proc root dir.\n");
		gpPr1000RootProcDir = proc_mkdir(strDevDrvName, NULL);

		//printk("Create proc fp /proc/pr1000/pr1000\n");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
		gpDrvHost->procFp = proc_create_data("pr1000", S_IFREG | S_IRWXU, gpPr1000RootProcDir, &pr1000ProcFileOps, (void *)gpDrvHost);
#else
		gpDrvHost->procFp = create_proc_entry("pr1000", S_IFREG | S_IRWXU, gpPr1000RootProcDir);
		if( gpDrvHost->procFp )
		{
			gpDrvHost->procFp->data = (void *)gpDrvHost;
			gpDrvHost->procFp->proc_fops = &pr1000ProcFileOps;
		}
#endif //LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)

		//printk("Create proc fp /proc/pr1000/log\n");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
		gpDrvHost->procFpLog = proc_create_data("log", S_IFREG | S_IRWXU, gpPr1000RootProcDir, &pr1000ProcLogFileOps, (void *)gpDrvHost);
#else
		gpDrvHost->procFpLog = create_proc_entry("log", S_IFREG | S_IRWXU, gpPr1000RootProcDir);
		if( gpDrvHost->procFpLog )
		{
			gpDrvHost->procFpLog->data = (void *)gpDrvHost;
			gpDrvHost->procFpLog->proc_fops = &pr1000ProcLogFileOps;
		}
#endif //LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)

		if( gpDrvHost->procFpLog )
		{
			if( (gpDrvHost->drvLog.pMem[0] = (char *)ZALLOC(DRV_LOG_MEM_LENGTH, GFP_KERNEL)) == NULL)
			{
				printk("ERROR: could not alloc pr1000 log memory.\n");
				goto Error_init_0;
			}
			gpDrvHost->drvLog.memLength[0] = (DRV_LOG_MEM_LENGTH/DRV_LOG_MEM_NUM);
			gpDrvHost->drvLog.wrPos[0] = 0;
			for(i = 1; i < DRV_LOG_MEM_NUM; i++)
			{
				gpDrvHost->drvLog.pMem[i] = (char *)gpDrvHost->drvLog.pMem[0] + (gpDrvHost->drvLog.memLength[0]*i);
				gpDrvHost->drvLog.memLength[i] = gpDrvHost->drvLog.memLength[0];
				gpDrvHost->drvLog.wrPos[i] = 0;
			}
			gpDrvHost->drvLog.logMemNum = 0;
		}
		//printk("Create proc fp /proc/pr1000/reg_dump\n");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
		gpDrvHost->procFpRegDump = proc_create_data("reg_dump", S_IFREG | S_IRWXU, gpPr1000RootProcDir, &fopsRegDump, gpDrvHost->procRegDumpStr0);
#else
		gpDrvHost->procFpRegDump = create_proc_entry("reg_dump", S_IFREG | S_IRWXU, gpPr1000RootProcDir);
		if(gpDrvHost->procFpRegDump)
		{
			gpDrvHost->procFpRegDump->data = gpDrvHost->procRegDumpStr0;
			gpDrvHost->procFpRegDump->read_proc = Pr1000ProcRdRegDumpFunc;
			gpDrvHost->procFpRegDump->write_proc = Pr1000ProcWrRegDumpFunc;
		}
#endif //LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)

		//printk("Create proc fp /proc/pr1000/reg_write\n");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
		gpDrvHost->procFpRegWrite = proc_create_data("reg_write", S_IFREG | S_IRWXU, gpPr1000RootProcDir, &fopsRegWrite, gpDrvHost->procRegWriteStr0);
#else
		gpDrvHost->procFpRegWrite = create_proc_entry("reg_write", S_IFREG | S_IRWXU, gpPr1000RootProcDir);
		if(gpDrvHost->procFpRegWrite)
		{
			gpDrvHost->procFpRegWrite->data = gpDrvHost->procRegWriteStr0;
			gpDrvHost->procFpRegWrite->read_proc = Pr1000ProcRdRegWriteFunc;
			gpDrvHost->procFpRegWrite->write_proc = Pr1000ProcWrRegWriteFunc;
		}
#endif //LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)

	}/*}}}*/
	PrintString("Support /proc/pr1000 system.\n");
#endif // SUPPORT_PROC_SYSTEM

	//printk("init ptz receive queue.\n");
	for(i = 0; i < MAX_CHN; i++) 
	{    
		if(PR1000_CQ_new(&gpDrvHost->ptzHost.ptzRecvQ[i], 1024))
		{    
			printk("ERROR: could not create ptz recv queue.\n");
			goto Error_init_0;
		}    
		PR1000_CQ_reset(&gpDrvHost->ptzHost.ptzRecvQ[i]);
		SPIN_LOCK_INIT(&gpDrvHost->ptzHost.splockPtzRecvQ[i]);

		/**< [20151222:OkHyun] */
		gpDrvHost->ptzHost.bRxUsedSTDFORMATOnly[i] = TRUE; //TRUE: PTZ Rx used check STDFORMAT only.
		//gpDrvHost->ptzHost.bRxUsedSTDFORMATOnly[i] = FALSE; //TRUE: PTZ Rx used check STDFORMAT only.
	}   

	PR1000_ChipInfo(0, gpDrvHost);
	for(i = PR1000_MASTER; i < PR1000_CHIP_COUNT; i++)
	{
		if( ((gpDrvHost->sysHost.chipID_verify>>i) & 1) == 0 ) 
		{
			Error("Invalid PR1000 ChipId : Chip%d \n", i );
			ret = -EINVAL;
			goto Error_init_0;
		}
	}

	if(PR1000_TestInfeface(0, gpDrvHost) < 0)
	{
		ErrorString("Invalid PR1000 i2c interface read & write test.\n");
		ret = -EINVAL;
		goto Error_init_0;
	}


	/* initialize each pr1000. do before interrupt handler. */
	if( PR1000_Init(0, gpDrvHost) < 0)
	{
		PrintString("Error: pr1000 driver initialize!\n");
		ret = -EINVAL;
		goto Error_init_0;
	}

	if(gpDrvHost->sysHost.chipID_verify & 1) 
	{
		gpDrvHost->irq = SetCPUExternalInterrupt();

		if(gpDrvHost->irq > 0)
		{
			Print("request irq:%d\n", gpDrvHost->irq);
			if( (request_irq(gpDrvHost->irq, IrqHandler, IRQF_SHARED|IRQF_DISABLED, "pr1000_drv", gpDrvHost)) != 0)
			{    
				Error("irq %d request failed: %d\n", gpDrvHost->irq COMMA ret);
				ret = -1;
				goto Error_init_0;
			}    
		}

		//////////////// run kernel thread ///////////////////////////
		Print("create kthread. Thread replace irq handler. Polling check irq every %dmsec\n", PR1000_THREAD_POLLING_PERIOD);
		gpKthreadId = kthread_run(PR1000_Kthread, (void *)gpDrvHost, "pr1000_kthread");
		if(gpKthreadId == NULL)
		{    
			ErrorString("fail create kthread(PR1000_Kthread)\n");
			goto Error_init_0;
		}
	}

	PrintString("Loading pr1000 driver is successful!\n");

	return 0;

Error_init_0:
        if(gpKthreadId)
        {    
		struct task_struct *task = gpKthreadId;
        	gpKthreadId = NULL;
                kthread_stop(task);
        }    
	if(gpDrvHost->irq)
	{
		free_irq(gpDrvHost->irq, (void *)gpDrvHost);
	}

	DbgString("remove ptz receive queue.\n");
	for(i = 0; i < MAX_CHN; i++) 
	{    
		PR1000_CQ_remove(&gpDrvHost->ptzHost.ptzRecvQ[i]);
		PR1000_CQ_delete(&gpDrvHost->ptzHost.ptzRecvQ[i]);
	}   

#ifdef SUPPORT_PROC_SYSTEM //support proc system for linux debugging.
	if(gpDrvHost->procFpRegDump)
	{
		remove_proc_entry("reg_dump", gpPr1000RootProcDir);
		gpDrvHost->procFpRegDump = NULL;
	}
	if(gpDrvHost->procFpRegWrite)
	{
		remove_proc_entry("reg_write", gpPr1000RootProcDir);
		gpDrvHost->procFpRegWrite = NULL;
	}
	if(gpDrvHost->procFpLog)
	{
		FREE(gpDrvHost->drvLog.pMem[0]);
		remove_proc_entry("log", gpPr1000RootProcDir);
		gpDrvHost->procFpLog = NULL;
	}
	if(gpDrvHost->procFp)
	{
		remove_proc_entry("pr1000", gpPr1000RootProcDir);
		gpDrvHost->procFp = NULL;
	}
	if(gpPr1000RootProcDir)
	{
		remove_proc_entry(strDevDrvName, NULL);
		gpPr1000RootProcDir = NULL;
	}
#endif // SUPPORT_PROC_SYSTEM

#ifdef CONFIG_HISI_SNAPSHOT_BOOT
	himedia_unregister(&himedia_pr1000Device);
#else
	misc_deregister(&pr1000_dev);	
#endif // CONFIG_HISI_SNAPSHOT_BOOT

	if(gpDrvHost)
	{
		FREE(gpDrvHost);
	}

	return(ret);
}
#else //#ifdef __LINUX_SYSTEM__
int pr1000_module_init(void)
{
	int ret = 0, i = 0;

	if( (gpDrvHost = (_drvHost *)&gDrvHost) == NULL)
	{
		ret = -1;
		ErrorString("ERROR: Can't allocate memory\n");
		return(ret);
	}
	memset(gpDrvHost, 0, sizeof(_drvHost));

	SPIN_LOCK_INIT(&gpDrvHost->spLockIrq);

	//DbgString("init ptz receive queue.\n");
	for(i = 0; i < MAX_CHN; i++) 
	{    
		if(PR1000_CQ_new(&gpDrvHost->ptzHost.ptzRecvQ[i], 1024))
		{    
			ErrorString("ERROR: could not create ptz recv queue.\n");
			goto Error_init_0;
		}    
		PR1000_CQ_reset(&gpDrvHost->ptzHost.ptzRecvQ[i]);
		SPIN_LOCK_INIT(&gpDrvHost->ptzHost.splockPtzRecvQ[i]);

		/**< [20151222:OkHyun] */
		gpDrvHost->ptzHost.bRxUsedSTDFORMATOnly[i] = TRUE; //TRUE: PTZ Rx used check STDFORMAT only.
		//gpDrvHost->ptzHost.bRxUsedSTDFORMATOnly[i] = FALSE; //TRUE: PTZ Rx used check STDFORMAT only.
	}   

	PR1000_ChipInfo(0, gpDrvHost);
	for(i = PR1000_MASTER; i < PR1000_CHIP_COUNT; i++)
	{
		if( ((gpDrvHost->sysHost.chipID_verify>>i) & 1) == 0 ) 
		{
			Error("Invalid PR1000 ChipId : Chip%d \n", i );
			ret = -1;
			goto Error_init_0;
		}
	}

	if(PR1000_TestInfeface(0, gpDrvHost) < 0)
	{
		ErrorString("Invalid PR1000 i2c interface read & write test.\n");
		ret = -1;
		goto Error_init_0;
	}


	/* initialize each pr1000. do before interrupt handler. */
	if( PR1000_Init(0, gpDrvHost) < 0)
	{
		PrintString("Error: pr1000 driver initialize!\n");
		ret = -1;
		goto Error_init_0;
	}
	if(gpDrvHost->sysHost.chipID_verify & 1) 
	{
		gpDrvHost->irq = SetCPUExternalInterrupt();

		if(gpDrvHost->irq > 0)
		{
			Print("request irq:%d\n", gpDrvHost->irq);
		}

		//////////////// run kernel thread ///////////////////////////
		Print("do kthread. Thread replace irq handler. Polling check irq every %dmsec\n", PR1000_THREAD_POLLING_PERIOD);
	}

	PrintString("Loading pr1000 driver is successful!\n");

	return 0;

Error_init_0:
	DbgString("remove ptz receive queue.\n");
	for(i = 0; i < MAX_CHN; i++) 
	{    
		PR1000_CQ_remove(&gpDrvHost->ptzHost.ptzRecvQ[i]);
		PR1000_CQ_delete(&gpDrvHost->ptzHost.ptzRecvQ[i]);
	}   

	return(ret);
}
#endif // __LINUX_SYSTEM__

#ifdef __LINUX_SYSTEM__
static void __exit pr1000_module_exit(void)
{
	int i;
	printk("pr1000 driver unregister!\n");
        if(gpKthreadId)
        {    
		struct task_struct *task = gpKthreadId;
        	gpKthreadId = NULL;
                kthread_stop(task);
        }    
	if(gpDrvHost->irq)
	{
		free_irq(gpDrvHost->irq, (void *)gpDrvHost);
	}

	DbgString("remove ptz receive queue.\n");
	for(i = 0; i < MAX_CHN; i++) 
	{    
		PR1000_CQ_remove(&gpDrvHost->ptzHost.ptzRecvQ[i]);
		PR1000_CQ_delete(&gpDrvHost->ptzHost.ptzRecvQ[i]);
	}   

#ifdef SUPPORT_PROC_SYSTEM //support proc system for linux debugging.
	if(gpDrvHost->procFpRegDump)
	{
		remove_proc_entry("reg_dump", gpPr1000RootProcDir);
		gpDrvHost->procFpRegDump = NULL;
	}
	if(gpDrvHost->procFpRegWrite)
	{
		remove_proc_entry("reg_write", gpPr1000RootProcDir);
		gpDrvHost->procFpRegWrite = NULL;
	}
	if(gpDrvHost->procFpLog)
	{
		FREE(gpDrvHost->drvLog.pMem[0]);
		remove_proc_entry("log", gpPr1000RootProcDir);
		gpDrvHost->procFpLog = NULL;
	}
	if(gpDrvHost->procFp)
	{
		remove_proc_entry("pr1000", gpPr1000RootProcDir);
		gpDrvHost->procFp = NULL;
	}
	if(gpPr1000RootProcDir)
	{
		remove_proc_entry(strDevDrvName, NULL);
		gpPr1000RootProcDir = NULL;
	}
#endif // SUPPORT_PROC_SYSTEM
#ifdef CONFIG_HISI_SNAPSHOT_BOOT
	himedia_unregister(&himedia_pr1000Device);
#else
	misc_deregister(&pr1000_dev);	
#endif // CONFIG_HISI_SNAPSHOT_BOOT

	if(gpDrvHost)
	{
		FREE(gpDrvHost);
	}

}
#else //#ifdef __LINUX_SYSTEM__
void pr1000_module_exit(void)
{
	int i;
	PrintString("pr1000 driver unregister!\n");

	DbgString("remove ptz receive queue.\n");
	for(i = 0; i < MAX_CHN; i++) 
	{    
		PR1000_CQ_remove(&gpDrvHost->ptzHost.ptzRecvQ[i]);
		PR1000_CQ_delete(&gpDrvHost->ptzHost.ptzRecvQ[i]);
	}   
}
#endif // __LINUX_SYSTEM__

#ifdef __LINUX_SYSTEM__
module_init(pr1000_module_init);
module_exit(pr1000_module_exit);

MODULE_LICENSE("GPL");
#endif // __LINUX_SYSTEM__

