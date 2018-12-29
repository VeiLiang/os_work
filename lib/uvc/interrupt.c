#include <string.h>
#include <xm_proj_define.h>
#include "common.h"
#include "gpio_function.h"
#include "descript_def.h"
#include "sensorlib.h"
#include "irqs.h"

#include <stdio.h>

#include <xm_core.h>
#include <xm_uvc_video.h>



bit bSuspendFlg = 0;
bit bResetFlg = 0;
bit bResumeFlg = 0;


BYTE xdata gbReset = 0;


extern void UVCStop (void);


void usb_endpoints_reset()
{	
	//endpoint 5 ISO IN (Video)
	unsigned int PacketSize;
	reg_usb_Index = VIDEO_EP_INDEX;
	MGC_Write8(MUSB_IDX_TXCSRL, TXCSRL_TxPktRdy);
	MGC_Write8(MUSB_IDX_TXCSRL, TXCSRL_FlushFIFO) ;
	MGC_Write8(MUSB_IDX_TXCSRL, TXCSRL_TxPktRdy);
	MGC_Write8(MUSB_IDX_TXCSRL, TXCSRL_FlushFIFO) ;
	//MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_FlushFIFO|TXCSRL_TxPktRdy) ;
	//MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_FlushFIFO|TXCSRL_TxPktRdy) ;
	//MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)&~TXCSRL_FIFONotEmpty) ;
	MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)&~TXCSRL_UnderRun) ;

	// 设置Video In端口的Packet Size
	//MGC_Write16(MUSB_IDX_TXMAXP, (USB20_INMAX_H_VIDEO_VAL << 8)|USB20_INMAX_L_VIDEO_VAL) ;
	PacketSize = (USB20_INMAX_H_VIDEO_VAL << 8)|USB20_INMAX_L_VIDEO_VAL;
	// PacketSize |= (4 - 1) << 11;
#if _VIDEO_EP_ == _VIDEO_EP_5_
	PacketSize |= (2 - 1) << 11;
#elif _VIDEO_EP_ == _VIDEO_EP_4_
	PacketSize |= (1 - 1) << 11;
#endif
	MGC_Write16(MUSB_IDX_TXMAXP, (unsigned short)PacketSize);
	
#if _PAYLOAD_PIPE_ == _PAYLOAD_PIPE_ISO_
	MGC_Write8(MUSB_IDX_TXCSRH, MGC_Read8(MUSB_IDX_TXCSRH)|TXCSRH_FrcDataTog) ;
	MGC_Write8(MUSB_IDX_TXCSRH, MGC_Read8(MUSB_IDX_TXCSRH)|TXCSRH_ISO) ;
#elif _PAYLOAD_PIPE_ == _PAYLOAD_PIPE_BULK_
	MGC_Write8(MUSB_IDX_TXCSRH, MGC_Read8(MUSB_IDX_TXCSRH) & ~(TXCSRH_ISO|TXCSRH_FrcDataTog)) ;
	MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_ClrDataTog) ;	// DATA0

#endif

	MGC_Write8(MUSB_IDX_TXCSRH, MGC_Read8(MUSB_IDX_TXCSRH)|TXCSRH_Mode) ;

	//endpoint 2 ISO IN (Audio)
	reg_usb_Index = 0x02;
	MGC_Write8(MUSB_IDX_TXCSRL, TXCSRL_TxPktRdy);
	MGC_Write8(MUSB_IDX_TXCSRL, TXCSRL_FlushFIFO) ;
	//MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_FlushFIFO|TXCSRL_TxPktRdy) ;
	//MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)&~TXCSRL_FIFONotEmpty) ;
	MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)&~TXCSRL_UnderRun) ;
	
	MGC_Write16(MUSB_IDX_TXMAXP, (USB_INMAX_H_AUDIO_VAL << 8)|USB_INMAX_L_AUDIO_VAL) ;
	
#if _PAYLOAD_PIPE_ == _PAYLOAD_PIPE_ISO_
	MGC_Write8(MUSB_IDX_TXCSRH, MGC_Read8(MUSB_IDX_TXCSRH)|TXCSRH_FrcDataTog) ;
	MGC_Write8(MUSB_IDX_TXCSRH, MGC_Read8(MUSB_IDX_TXCSRH)|TXCSRH_ISO) ;
#elif _PAYLOAD_PIPE_ == _PAYLOAD_PIPE_BULK_
	MGC_Write8(MUSB_IDX_TXCSRH, MGC_Read8(MUSB_IDX_TXCSRH) & ~(TXCSRH_ISO|TXCSRH_FrcDataTog)) ;
#endif
	MGC_Write8(MUSB_IDX_TXCSRH, MGC_Read8(MUSB_IDX_TXCSRH)|TXCSRH_Mode) ;
	
	// endpoint1 IN (interrupt)
	reg_usb_Index = 0x01;
	MGC_Write16(MUSB_IDX_TXMAXP, (0x00 << 8)|0x40) ;
	MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_FlushFIFO) ;
	MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_FlushFIFO) ;
	MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)&~TXCSRL_FIFONotEmpty) ;
	MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)&~TXCSRL_UnderRun) ;
	MGC_Write8(MUSB_IDX_TXCSRH, MGC_Read8(MUSB_IDX_TXCSRH)&~TXCSRH_FrcDataTog) ;
	MGC_Write8(MUSB_IDX_TXCSRH, MGC_Read8(MUSB_IDX_TXCSRH)&~TXCSRH_ISO) ;
	MGC_Write8(MUSB_IDX_TXCSRH, MGC_Read8(MUSB_IDX_TXCSRH)|TXCSRH_Mode) ;
	MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_ClrDataTog) ;
	MGC_Write8(MUSB_IDX_TXCSRH, MGC_Read8(MUSB_IDX_TXCSRH)&~TXCSRH_Mode) ;

	//reg_usb_IntrInE = 0x0D;

	reg_usb_Index = 0;
}

extern  void set_soft_reboot (int second_to_timeout);
extern  void clr_soft_reboot (void);

void usb_reset (void)
{	
	reg_usb_Index = 0;
	reg_usb_Power &= (~0x01);	//需要软件清除操作
	
	bResetFlg = 1;

	reg_usb_IntrUSBE = 0x07;		//0x04; //reset/resume/suspend int enable
	
	gsDeviceStatus.bUsb_DevState= DEVSTATE_DEFAULT;
	gsEp0Status.bState = EP0_IDLE;
	gsEp0Status.bTxLock = FALSE;			//
	gsEp0Status.wBytesLeft = 0;				//
	gsEp0Status.bTestMode = 0x00;			//
	gsEp0Status.bFAddr = 0xFF;

	reg_usb_EP0 |= CSR0L_ServicedRxPktRdy;
	
	reg_usb_Index = VIDEO_EP_INDEX;	//有必要在这里flush  吗???
	MGC_Write8(MUSB_IDX_TXCSRL, TXCSRL_TxPktRdy);
	MGC_Write8(MUSB_IDX_TXCSRL, TXCSRL_FlushFIFO) ;
	MGC_Write8(MUSB_IDX_TXCSRL, TXCSRL_TxPktRdy);
	MGC_Write8(MUSB_IDX_TXCSRL, TXCSRL_FlushFIFO) ;
	//MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_FlushFIFO|TXCSRL_TxPktRdy) ;
	//MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_FlushFIFO|TXCSRL_TxPktRdy) ;
	
	bSuspendFlg = 0;
	gbReset = 0;
	
	//XM_printf ("RESET UVCStop\n");
	//UvcBreak ();
		
	// 清除软件复位标志
	//clr_soft_reboot ();
}

int is_usb_suspend (void)
{
	return bSuspendFlg;
}

void usb_resume()
{
	bSuspendFlg = 0;
	XM_printf ("usb_resume\n");
}

void usb_suspend()
{	
	bSuspendFlg = 1;
	UvcBreak ();
	XM_printf ("usb_suspend\n");
}

void usb_read_cmd()			//读取USB STANDARD命令
{
	BYTE  bTemp = 0;

 	gsUsbRequest.bmRequestType = reg_usb_End0;
	gsUsbRequest.bRequest = reg_usb_End0;
	bTemp = reg_usb_End0;
	gsUsbRequest.wValue = ((WORD)reg_usb_End0<<8) + bTemp;
	bTemp = reg_usb_End0;
	gsUsbRequest.wIndex = ((WORD)reg_usb_End0<<8) + bTemp;
	bTemp = reg_usb_End0;
	gsUsbRequest.wLength = ((WORD)reg_usb_End0<<8) + bTemp;
	//bTemp = reg_usb_End0;	//需不需要读取最后一个字节？

	//修改在每次取描述表的时候清零BytesLeft
	if ((gsUsbRequest.bmRequestType == 0x80) && (gsUsbRequest.bRequest == 0x06))
		gsEp0Status.wBytesLeft = 0x0000;

}

void usb_ep0_isr()
{
	reg_usb_Index = 0;
	//XM_printf ("ISR reg_usb_EP0=0x%x\n", reg_usb_EP0);
	if(!(reg_usb_EP0 & CSR0L_RxPktRdy))
	{
		//没有数据包,  故为Status Phase
		if (gsEp0Status.bFAddr != 0xFF)	//Complete SET_ADDRESS command
		{
			reg_usb_Faddr = gsEp0Status.bFAddr;
	
			if ((gsDeviceStatus.bUsb_DevState == DEVSTATE_DEFAULT) && gsEp0Status.bFAddr)
				gsDeviceStatus.bUsb_DevState = DEVSTATE_ADDRESS;
			else if ((gsDeviceStatus.bUsb_DevState == DEVSTATE_ADDRESS) && !gsEp0Status.bFAddr)
				gsDeviceStatus.bUsb_DevState = DEVSTATE_DEFAULT;
	
			gsEp0Status.bFAddr = 0xFF;	/* Clear pending commands */
		}
	
		if (gsEp0Status.bTestMode)
		{
			if (gsEp0Status.bTestMode == 0x08)	//Test_PKT
			{
				BYTE bCount;
				BYTE code* pSrc;
				bCount = 53;
				pSrc = &gcTestPacket[0];
				
				while (bCount)
				{
					reg_usb_End0 = *pSrc++;
					bCount--;
				}
				reg_usb_EP0 |= CSR0L_TxPktRdy;		
			}
			reg_usb_Testmode = gsEp0Status.bTestMode;
	
			gsEp0Status.bTestMode = 0;	/* Clear pending commands */
		}
	}
	
	if (reg_usb_EP0 & CSR0L_SentStall) 	//Check for SentStall
	{
		reg_usb_EP0 &= ~CSR0L_SentStall;
		gsEp0Status.bState = EP0_IDLE;
		// EP0应该恢复到复位状态
		gsEp0Status.bTxLock = FALSE;
	}
	
	if (reg_usb_EP0 & CSR0L_SetupEnd)	//reg_EP0_SetupEnd
	{
		// This bit will be set when a control transaction ends before the DataEnd bit has been set.
		// An interrupt will be generated and the FIFO flushed at this time. The bit is cleared by
		// the CPU writing a 1 to the ServicedSetupEnd bit.
		// reg_usb_EP0 |= CSR0L_ServicedSetupEnd;
		reg_usb_EP0 |= CSR0L_ServicedSetupEnd;
		gsEp0Status.bState = EP0_IDLE;

		// EP0应该恢复到复位状态
		gsEp0Status.bTxLock = FALSE;
	}
	
	if (gsEp0Status.bState == EP0_IDLE)
	{
		/* If no packet has been received, assume that this was a STATUS phase complete. */
		/* Otherwise load new command */
		if(reg_usb_EP0 & CSR0L_RxPktRdy)	
		{
			//XM_printf ("reg_usb_EP0=%x\n", reg_usb_EP0);
			if (gbReset == 0)
			{
				gbReset = 1;
				// reg_usb_IntrInE = 0x0D;
				// reg_usb_IntrInE = 0x25;
				reg_usb_IntrInE = 0x05 | (1 << VIDEO_EP_INDEX);
				usb_endpoints_reset ();
			}

			usb_read_cmd();
			gsEp0Status.bState = EP0_CMD;
		}
	}
	
	if (gsEp0Status.bState == EP0_TX)
	{
		gsEp0Status.bTxLock = FALSE;		//让usb_ep0_transfer()  继续发包
	}
	
	/*
	if (gsEp0Status.bState == EP0_RX)
	{
		if(reg_usb_EP0 & CSR0L_RxPktRdy)		
			gsEp0Status.bState = EP0_IDLE;
	}*/
}


extern 	void UVCTransfer (void);

void usb_interrupt_rom (void)
{
	BYTE bIntrUsb, bIntrIn;
	BYTE bIdxTemp;
	
	// usb_interrupt_ticket = XM_GetTickCount ();
	
	bIdxTemp = reg_usb_Index;
	reg_usb_Index = 0;
	bIntrUsb = reg_usb_IntrUSB;	//read the usb interrupt register, d3 - SOF; d2 - reset; d1 - resume; d0 - suspend
	bIntrIn = reg_usb_IntrIn;	//d3 - ep3, video; d2 - ep2, audio; d1 - ep1; d0 - ep0

	if (bIntrUsb & 0x02)	//resume interrupt
	{
		//XM_printf ("usb_resume\n");
		usb_resume ();
		//(*pFunc_usb_resume)();	//usb_resume();
	}


	if (bIntrUsb & 0x04)	// reset interrupt
	{		
		XM_printf ("usb_reset\n");
		usb_reset ();
		//(*pFunc_usb_reset)();	//usb_reset();
	}

	if (bIntrIn & 0x01)		//ep0 usb control interrupt
	{
		extern void XMSYS_CameraNotifyUsbEvent (void);

		usb_ep0_isr();
		//XMSYS_CameraPostEvent (XMSYS_CAMERA_USB_EVENT);
		XMSYS_CameraNotifyUsbEvent ();
	}

	if (bIntrIn & 0x04)							//ep2 audio interrupt
	{
		// XM_printf ("ep2 audio interrupt\n");
		if (gsAudioStatus.bStartAudio == TRUE)
		{
			reg_usb_Index = 0x02;
			MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_TxPktRdy) ;	
		}
	}
	
	if (bIntrIn & (0x01 << VIDEO_EP_INDEX))							//ep5 video interrupt
	// if (bIntrIn & 0x20)							//ep5 video interrupt
	{
		BYTE TxCSRL, TxCSRH;
		reg_usb_Index = VIDEO_EP_INDEX;	
		TxCSRH = MGC_Read8(MUSB_IDX_TXCSRH);
		TxCSRL = MGC_Read8(MUSB_IDX_TXCSRL);
		//XM_printf ("TxCSRL=%x, TxCSRH=%x\n", TxCSRL, TxCSRH);
		if(TxCSRL & TXCSRL_UnderRun)
		{
			TxCSRL &= ~TXCSRL_UnderRun;
			MGC_Write8(MUSB_IDX_TXCSRL, TxCSRL);
		}

		if(TxCSRL & TXCSRL_SentStall)
		{
			XM_printf ("STALL handshake is transmitted\n");
			TxCSRL &= ~TXCSRL_SentStall;
			MGC_Write8(MUSB_IDX_TXCSRL, TxCSRL);
			
		}
		
		//XM_printf ("ep5 video interrupt\n");
		if (gsVideoStatus.bStartVideo)
		{
			UVCTransfer ();
		}
		else
		{
		}
	}
	
	if (bIntrUsb & 0x01)	//suspend interrupt
	{
		//XM_printf ("suspend\n");
		usb_suspend();
		//(*pFunc_usb_suspend)();	//usb_suspend();
	}
	reg_usb_Index = bIdxTemp;
}



void usb_interrupt(void)
{
	usb_interrupt_rom();
}

extern void uvc_usb_dma_transfer_finish_interrupt (void);
void usb_dma_interrupt (void)
{
	uvc_usb_dma_transfer_finish_interrupt ();
}













