//****************************************************************************
//
//	Copyright (C) 2014 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: arkn141_codec.c
//
//	Revision history
//
//		2014.12.21	ZhuoYongHong Initial version
//
//		H264 encoder/decoder 及 JPEG encoder/decoder 的硬件资源存在共享，
//		执行过程中需要保护
//
//****************************************************************************
#include <stdio.h>
#include "ark1960.h"
#include "rtos.h"
#include "arkn141_codec.h"
#include "common.h"
#include "xm_type.h"
#include "xm_dev.h"
#include "timer.h"
#include "sys_soft_reset.h"
#include "cpuClkCounter.h"

#define	DEBUG

#ifdef DEBUG
#define	DEBUG_PRINT(fmt, args...)		printf(fmt, ## args)	
#else
#define	DEBUG_PRINT(fmt, args...)	
#endif

extern int  hx280enc_init(void);
extern void hx280enc_exit(void);
extern int  hx170dec_init(void);
extern void hx170dec_cleanup(void);


static OS_RSEMA arkn141_codec_semid;
static volatile unsigned int arkn141_codec_inited = 0;
static  unsigned int arkn141_codec_opened[2];
static volatile int codec_share_memory_user;		// 0 	编码器
														// 1 	解码器		
														// -1	未使用

static OS_EVENT h264_encode_event;			// H264编码器中断事件
														//		由H264编码器中断驱动，编码器等待该事件
static OS_EVENT codec_decode_event;			// 解码器中断事件
														//		由解码器中断驱动，解码器等待该事件

// 编码时钟约束 3.2ns	312.5MHz
// 解码时钟约束 3.9ns	256MHz
// ARKN141 硬件编解码器初始化，分配系统资源
int arkn141_codec_init (void)
{
	unsigned int val;
	int ret = -1;
	OS_EnterRegion ();
	if(!arkn141_codec_inited)
	{
		OS_EVENT_Create (&h264_encode_event);
		OS_EVENT_Create (&codec_decode_event);
		OS_CREATERSEMA (&arkn141_codec_semid);
		arkn141_codec_inited = 1;
		memset (arkn141_codec_opened, 0, sizeof(arkn141_codec_opened));
		codec_share_memory_user = -1;
		
		sys_clk_disable (Mae2_enc_enable);
		sys_clk_disable (Mae2_dec_enable);
		
		// 复位前必须关闭时钟
#if 1
		sys_clk_disable (Mae2_enc_enable);
		sys_clk_disable (Mae2_dec_enable);
		sys_clk_disable (mae_xclk_enable);
		delay (100);
		sys_soft_reset (softreset_mae2_core);
		sys_soft_reset (softreset_mae2_axi);
#endif
		//sys_clk_enable (Mae2_enc_enable);
		//sys_clk_enable (Mae2_dec_enable);
		sys_clk_enable (mae_xclk_enable);
		
#if 1
		// 初始化硬件编码器时钟
		// SYS_DEVICE_CLK_CFG2
		unsigned int mfc_enc_clk_div;
		unsigned int Int_mfc_enc_clk_sel;
		
		// AXI 300M  AUD 400M ENC 400M
		
		// 25-23	R/W	0x0	mfc enc clk div
		// 						mfc enc clk = int_mfc_enc_clk/div
		mfc_enc_clk_div = 1;
		
		// 22-20	R/W	0x4	Int_mfc_enc_clk_sel
		//							4'b0000: Clk_240m
		//							4'b0001: Syspll_clk
		//							4'b0010: audpll_clk
		//							4'b0100: Cpupll_clk
		Int_mfc_enc_clk_sel = 2;
		//Int_mfc_enc_clk_sel = 1;
		// 编码器的时钟尽可能选择较高的时钟,这样h264帧编码时间会缩短.
		
		XM_lock ();
		val = rSYS_DEVICE_CLK_CFG2;
		val &= ~((0x7 << 20) | (0x7 << 23));
		val |=  (Int_mfc_enc_clk_sel << 20) | (mfc_enc_clk_div << 23);		// 
		rSYS_DEVICE_CLK_CFG2 = val;
		XM_unlock ();
#endif
				
#if 1
				// 初始化硬件解码器时钟
				// 解码器的时钟不要超过310MHz, 会出现花屏/总线错误/解码挂死等异常
				// SYS_DEVICE_CLK_CFG2
				unsigned int mfc_dec_clk_div;
				unsigned int Int_mfc_dec_clk_sel;
				
				// 1) AXI 300M AUD 400M DEC 150
				// 2) AXI 300M AUD 400M DEC 300
				
				// 19-17	R/W	0x0	Mfc_dec_clk div
				// Mfc_dec clk = int_mfc_dec_clk/div
				mfc_dec_clk_div = 1;
				
				// 16-14	R/W	0x4	Int_mfc_dec_clk_sel
				// 						4'b0000: Clk_240m
				//							4'b0001: Syspll_clk
				//							4'b0010: audpll_clk
				//							4'b0100: Cpupll_clk
				//Int_mfc_dec_clk_sel = 2;	// audpll_clk
				Int_mfc_dec_clk_sel = 1;	// Syspll_clk
				//Int_mfc_dec_clk_sel = 0;	// 240MHz
				
				XM_lock ();
				val = rSYS_DEVICE_CLK_CFG2;
				val &= ~((0x7 << 14) | (0x7 << 17));
				val |=  (Int_mfc_dec_clk_sel << 14) | (mfc_dec_clk_div << 17);		// 
				rSYS_DEVICE_CLK_CFG2 = val;
				XM_unlock ();
#endif
				
		printf ("mfc enc clock = %d, dec clock = %d\n", arkn141_get_clks(ARKN141_CLK_H264_ENCODER), arkn141_get_clks(ARKN141_CLK_H264_DECODER));
				
		ret = 0;
	}
	else
	{
		DEBUG_PRINT ("arkn141_codec_init initialized before.\n");
	}
	OS_LeaveRegion ();
	return ret;
}

// ARKN141 硬件编解码器关闭，释放系统资源
int arkn141_codec_exit (void)
{
	int ret = -1;
	OS_EnterRegion ();
	if(arkn141_codec_inited)
	{
		OS_EVENT_Delete (&h264_encode_event);
		OS_EVENT_Delete (&codec_decode_event);
		OS_DeleteRSema (&arkn141_codec_semid);
		arkn141_codec_inited = 0;
		ret = 0;
	}
	else
	{
		DEBUG_PRINT ("arkn141_codec_init un-initialized before.\n");
	}
	OS_LeaveRegion ();
	return ret;
}


// 打开硬件编解码器
int arkn141_codec_open  (int codec_type)
{
	if(codec_type >= ARKN141_CODEC_TYPE_COUNT)
		return -1;
	OS_EnterRegion ();
	
	/*
	sys_clk_disable (Mae2_enc_enable);
	sys_clk_disable (Mae2_dec_enable);
	sys_clk_disable (mae_xclk_enable);
	delay (100);
	sys_soft_reset (softreset_mae2_core);
	sys_soft_reset (softreset_mae2_axi);
	sys_clk_enable (Mae2_enc_enable);
	sys_clk_enable (Mae2_dec_enable);
	sys_clk_enable (mae_xclk_enable);
	*/
	
	//printf ("arkn141_codec_open %d\n", codec_type);
	switch (codec_type)
	{
		case ARKN141_CODEC_TYPE_H264_ENCODER:
		case ARKN141_CODEC_TYPE_JPEG_ENCODER:
			// 检查是否开启
			if(arkn141_codec_opened[0] == 0)
			{
				sys_clk_enable (Mae2_enc_enable);
				hx280enc_init ();
			}
			arkn141_codec_opened[0] ++;
			break;
			
		case ARKN141_CODEC_TYPE_H264_DECODER:
		case ARKN141_CODEC_TYPE_JPEG_DECODER:
			if(arkn141_codec_opened[1] == 0)
			{
				sys_clk_enable (Mae2_dec_enable);
				// 初始化硬件解码器
				hx170dec_init ();
			}
			arkn141_codec_opened[1] ++;
			break;
	}
	OS_LeaveRegion ();
	return 0;
}

// 关闭硬件编解码器
int arkn141_codec_close (int codec_type)
{
	int ret = 0;
	if(codec_type >= ARKN141_CODEC_TYPE_COUNT)
		return -1;
	OS_EnterRegion ();
	//printf ("arkn141_codec_close %d\n", codec_type);
	switch (codec_type)
	{
		case ARKN141_CODEC_TYPE_H264_ENCODER:
		case ARKN141_CODEC_TYPE_JPEG_ENCODER:
			// 检查是否开启
			if(arkn141_codec_opened[0] == 0)
			{
				ret = -1;
				DEBUG_PRINT ("error, encoder HW closed before\n");
				break;
			}
			arkn141_codec_opened[0] --;
			if(arkn141_codec_opened[0] == 0)
			{
				// 关闭硬件编码器
				hx280enc_exit ();
				//sys_clk_disable (Mae2_enc_enable);
			}
			break;
			
		case ARKN141_CODEC_TYPE_H264_DECODER:
		case ARKN141_CODEC_TYPE_JPEG_DECODER:
			if(arkn141_codec_opened[1] == 0)
			{
				ret = -1;
				DEBUG_PRINT ("error, decoder HW closed before\n");
				break;
			}
			arkn141_codec_opened[1] --;
			if(arkn141_codec_opened[1] == 0)
			{
				// 关闭硬件解码器
				hx170dec_cleanup ();
				// 20170505 zhuoyonghong 
				// 不要在此处关闭Mae2_dec_enable时钟. 
				//	当解码器出现某些异常时, 如(IRQ: BUFFER EMPTY), 关闭解码器时钟会导致系统挂死.
				//sys_clk_disable (Mae2_dec_enable);
				// 20171025 zhuoyonghong
				// 不关闭Mae2_dec_enable会导致解码器空闲时继续消耗20~30ma电流
				// 延时关闭
				//OS_Delay (100);
				//sys_clk_disable (Mae2_dec_enable);
			}
			break;
	}
	OS_LeaveRegion ();
	return ret;
}

// 请求对硬件codec资源的互斥使用(排他性使用)
int arkn141_codec_reserve_hw (int b_is_decoder)
{
	if(!arkn141_codec_inited)
	{
		DEBUG_PRINT ("arkn141_codec_init un-initialized before.\n");
		return -1;
	}
	OS_Use (&arkn141_codec_semid);
	if(codec_share_memory_user != (-1))
	{
		OS_Unuse (&arkn141_codec_semid);
		DEBUG_PRINT ("revserd hw Error, share memory used by %d before.\n", 
						 codec_share_memory_user);
		return -1;
	}
	// 0表示decode
	// 1表示encode
	XM_lock ();
	rSYS_DEVICE_CLK_CFG0 &=~(0x3<<28);
	if(!b_is_decoder)
		rSYS_DEVICE_CLK_CFG0 |= (0x1<<28);
	XM_unlock ();
	
	
	// 21	R/W	1	softreset_mae2_core
	//XM_lock ();
	//rSYS_SOFT_RSTNA &= ~((1 << 21) | (1 << 6));
	//XM_unlock ();
	//delay (100);
	//XM_lock ();
	//rSYS_SOFT_RSTNA |=  (1 << 21) | (1 << 6);
	//XM_unlock ();
	//delay (100);
	if(b_is_decoder)
		OS_EVENT_Reset (&codec_decode_event);
	else
		OS_EVENT_Reset (&h264_encode_event);
	codec_share_memory_user = b_is_decoder;
	return 0;
}

// 释放对硬件codec资源的互斥使用(排他性使用)
int arkn141_codec_release_hw (int b_is_decoder)
{
	if(!arkn141_codec_inited)
	{
		DEBUG_PRINT ("arkn141_codec_init un-initialized before.\n");
		return -1;
	}
	if(codec_share_memory_user != b_is_decoder)
	{
		DEBUG_PRINT ("revserd hw Error, share memory used by %d before.\n", 
						 codec_share_memory_user);
		return -1;
	}
	codec_share_memory_user = -1;
	//XM_lock ();
	//rSYS_DEVICE_CLK_CFG0 &=~(0x3<<28);
	//XM_unlock ();
	OS_Unuse (&arkn141_codec_semid);
	return 0;
}

// 等待H264编码器完成
// 0   h264 codec encode finish
// -1  h264 codec 未初始化
// -2  h264 codec timeout
int arkn141_codec_waiting_for_h264_encode_finished (void)
{
	OS_TIME Timeout = 1000;
	if(!arkn141_codec_inited)
	{
		DEBUG_PRINT ("arkn141_codec_init un-initialized before.\n");
		return -1;
	}
	
	if(OS_EVENT_WaitTimed (&h264_encode_event, Timeout) == 0)
	{
		// 0: Success, the event was signaled within the specified time.
		return 0;
	}
	else
	{
		// 1: The event was not signaled and a timeout occurred.
		printf ("h264 /jpeg encode timeout\n");
		return -2;
	}
}

// 触发H264编码完成事件
void arkn141_codec_trigger_h264_encode_finish_event (void)
{
	OS_EVENT_Set (&h264_encode_event);
}

// 等待解码器解码完成
// timeout	   timeout period for the wait specified in milliseconds; 
// -1          means an infinit wait   
// 返回值定义
// 0   codec decode finish
// -1  codec 未初始化
// -2  codec timeout
int arkn141_codec_waiting_for_decode_finished (unsigned int timeout)
{
	int infinit_wait = 0;
	if(!arkn141_codec_inited)
	{
		DEBUG_PRINT ("arkn141_codec_init un-initialized before.\n");
		return -1;
	}
	
	if(timeout == (unsigned int)(-1))
	{
		// 20180711 zhuoyonghong
		// 未避免解码异常导致无限等待，关闭无限等待
		infinit_wait = 1;
		timeout = 3000;	// 不允许无限等待
		//timeout = 2000;	// 不允许无限等待
		//OS_EVENT_Wait (&codec_decode_event);
		//return 0;
	}
	
	if(OS_EVENT_WaitTimed (&codec_decode_event, timeout) == 0)
	{
		// 0: Success, the event was signaled within the specified time.
		return 0;
	}
	else
	{
		if(infinit_wait == 1)
			printf ("h264 /jpeg decode infinit_wait\n");
		// 1: The event was not signaled and a timeout occurred.
		printf ("h264 /jpeg decode timeout\n");
		return -2;
	}
}

// 触发解码器解码完成事件
void arkn141_codec_trigger_decode_finish_event (void)
{
	OS_EVENT_Set (&codec_decode_event);
}
