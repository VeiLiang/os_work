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
//		H264 encoder/decoder �� JPEG encoder/decoder ��Ӳ����Դ���ڹ���
//		ִ�й�������Ҫ����
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
static volatile int codec_share_memory_user;		// 0 	������
														// 1 	������		
														// -1	δʹ��

static OS_EVENT h264_encode_event;			// H264�������ж��¼�
														//		��H264�������ж��������������ȴ����¼�
static OS_EVENT codec_decode_event;			// �������ж��¼�
														//		�ɽ������ж��������������ȴ����¼�

// ����ʱ��Լ�� 3.2ns	312.5MHz
// ����ʱ��Լ�� 3.9ns	256MHz
// ARKN141 Ӳ�����������ʼ��������ϵͳ��Դ
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
		
		// ��λǰ����ر�ʱ��
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
		// ��ʼ��Ӳ��������ʱ��
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
		// ��������ʱ�Ӿ�����ѡ��ϸߵ�ʱ��,����h264֡����ʱ�������.
		
		XM_lock ();
		val = rSYS_DEVICE_CLK_CFG2;
		val &= ~((0x7 << 20) | (0x7 << 23));
		val |=  (Int_mfc_enc_clk_sel << 20) | (mfc_enc_clk_div << 23);		// 
		rSYS_DEVICE_CLK_CFG2 = val;
		XM_unlock ();
#endif
				
#if 1
				// ��ʼ��Ӳ��������ʱ��
				// ��������ʱ�Ӳ�Ҫ����310MHz, ����ֻ���/���ߴ���/����������쳣
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

// ARKN141 Ӳ����������رգ��ͷ�ϵͳ��Դ
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


// ��Ӳ���������
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
			// ����Ƿ���
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
				// ��ʼ��Ӳ��������
				hx170dec_init ();
			}
			arkn141_codec_opened[1] ++;
			break;
	}
	OS_LeaveRegion ();
	return 0;
}

// �ر�Ӳ���������
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
			// ����Ƿ���
			if(arkn141_codec_opened[0] == 0)
			{
				ret = -1;
				DEBUG_PRINT ("error, encoder HW closed before\n");
				break;
			}
			arkn141_codec_opened[0] --;
			if(arkn141_codec_opened[0] == 0)
			{
				// �ر�Ӳ��������
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
				// �ر�Ӳ��������
				hx170dec_cleanup ();
				// 20170505 zhuoyonghong 
				// ��Ҫ�ڴ˴��ر�Mae2_dec_enableʱ��. 
				//	������������ĳЩ�쳣ʱ, ��(IRQ: BUFFER EMPTY), �رս�����ʱ�ӻᵼ��ϵͳ����.
				//sys_clk_disable (Mae2_dec_enable);
				// 20171025 zhuoyonghong
				// ���ر�Mae2_dec_enable�ᵼ�½���������ʱ��������20~30ma����
				// ��ʱ�ر�
				//OS_Delay (100);
				//sys_clk_disable (Mae2_dec_enable);
			}
			break;
	}
	OS_LeaveRegion ();
	return ret;
}

// �����Ӳ��codec��Դ�Ļ���ʹ��(������ʹ��)
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
	// 0��ʾdecode
	// 1��ʾencode
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

// �ͷŶ�Ӳ��codec��Դ�Ļ���ʹ��(������ʹ��)
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

// �ȴ�H264���������
// 0   h264 codec encode finish
// -1  h264 codec δ��ʼ��
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

// ����H264��������¼�
void arkn141_codec_trigger_h264_encode_finish_event (void)
{
	OS_EVENT_Set (&h264_encode_event);
}

// �ȴ��������������
// timeout	   timeout period for the wait specified in milliseconds; 
// -1          means an infinit wait   
// ����ֵ����
// 0   codec decode finish
// -1  codec δ��ʼ��
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
		// δ��������쳣�������޵ȴ����ر����޵ȴ�
		infinit_wait = 1;
		timeout = 3000;	// ���������޵ȴ�
		//timeout = 2000;	// ���������޵ȴ�
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

// ������������������¼�
void arkn141_codec_trigger_decode_finish_event (void)
{
	OS_EVENT_Set (&codec_decode_event);
}
