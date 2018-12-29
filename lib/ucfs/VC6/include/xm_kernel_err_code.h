//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_kernel_err_code.h
//	  constant，macro & basic typedef definition of kernel error definition
//
//	Revision history
//
//		2010.09.01	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_KERNEL_ERR_CODE_H_
#define _XM_KERNEL_ERR_CODE_H_

#if defined (__cplusplus)
	extern "C"{
#endif

// 内核报告的无法恢复的异常
#define	XM_KERNEL_ERR_CODE_SD_DMA_CHANNEL_REQUEST		(-1)		// sd read dma channel 
#define	XM_KERNEL_ERR_CODE_SD_DMA_LLI_MALLOC			(-2)		// LLI malloc failed

void XM_TraceKernelErrCode (int nErrCode);	


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_KERNEL_ERR_CODE_H_
