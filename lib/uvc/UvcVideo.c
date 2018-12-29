//****************************************************************************
//
//	Copyright (C) 2013 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: UvcVideo.c
//
//	Revision history
//
//		2013.5.1 ZhuoYongHong Initial version
//
//****************************************************************************
#include <xm_proj_define.h>
#include <string.h>
#include <stdio.h>
#include "xm_uvc_video.h"
#include "common.h"
#include "descript_def.h"
#include "sensorlib.h"
#include <xm_core.h>
#include <assert.h>

#define	PAYLOAD_HEADER_SIZE	4

extern void delay(unsigned int time);
static void uvc_dma_stop (void);

// Video In端口的Maximum Packet Size
extern BYTE USB20_INMAX_L_VIDEO_VAL;
extern BYTE USB20_INMAX_H_VIDEO_VAL;

static struct UVCVIDEOSAMPLE UVCVideoSample;
static volatile int UVCVideoReady = 0;

// JPEG窗口分辨率索引
static volatile unsigned int reg_jpegWinIdx;


void jpeg_reg_1_2_3_set(BYTE bPara)
{
	reg_jpegWinIdx = bPara;
}


static void burst_load_fifo (volatile unsigned char *addr, unsigned char *data, unsigned int count)
{
#if 1
	// 使用ARM的LDM指令优化
	asm (	"stmdb	sp!, {r4, r5, r6}\n"
			"B  	_burst_load_fifo_0\n"
		  	"burst_load_fifo_1:\n"
			"LDMIA	r1!, {r3, r4, r5, r6}\n"
			"SUBS r2, 	r2, #16\n"
			"STR	r3,	[r0]\n"
			"STR	r4,	[r0]\n"
			"STR	r5,	[r0]\n"
			"STR	r6,	[r0]\n"
			"_burst_load_fifo_0:\n"
			"CMP	r2, 	#16\n"
		 	"BCS	burst_load_fifo_1\n" 
			"ldmia	sp!, {r4, r5, r6}\n"
			
			"BX	lr"		  );
	
#else
	while(count >= 16)
	{
		*(volatile unsigned int *)addr = *(unsigned int *)data;
		*(volatile unsigned int *)addr = *(unsigned int *)(data+4);
		*(volatile unsigned int *)addr = *(unsigned int *)(data+8);
		*(volatile unsigned int *)addr = *(unsigned int *)(data+12);
		data += 16;
		count -= 16;
	}
#endif
}

#define MGC_MIN(_n1_, _n2_) (_n2_ < _n1_ ? _n2_ : _n1_)
#define MGC_MAX(_n1_, _n2_) (_n2_ > _n1_ ? _n2_ : _n1_)

/****************************** CONSTANTS ********************************/

#define MGC_O_HSDMA_BASE			0x200
#define MGC_O_HSDMA_INTR			0x200

#define MGC_O_HSDMA_CONTROL		4
#define MGC_O_HSDMA_ADDRESS		8
#define MGC_O_HSDMA_COUNT			0xc

#define MGC_HSDMA_CHANNEL_OFFSET(_bChannel, _bOffset) (MGC_O_HSDMA_BASE + (_bChannel << 4) + _bOffset)

/* control register (16-bit): */
#define MGC_S_HSDMA_ENABLE				0
#define MGC_S_HSDMA_TRANSMIT			1
#define MGC_S_HSDMA_MODE1				2
#define MGC_S_HSDMA_IRQENABLE			3
#define MGC_S_HSDMA_ENDPOINT			4
#define MGC_S_HSDMA_BUSERROR			8
#define MGC_S_HSDMA_BURSTMODE			9
#define MGC_M_HSDMA_BURSTMODE			(3 << MGC_S_HSDMA_BURSTMODE)
#define MGC_HSDMA_BURSTMODE_UNSPEC  	0
#define MGC_HSDMA_BURSTMODE_INCR4   	1
#define MGC_HSDMA_BURSTMODE_INCR8   	2
#define MGC_HSDMA_BURSTMODE_INCR16  	3

unsigned int usb_dma_short_package;
static void bulkLoadFifo_payload(uint8_t bEnd, uint16_t wCount, const uint8_t* pSource)
{
	unsigned int old_TxCSRH;
	unsigned int bIdxTemp;
	
	//printf ("usb dma 0x%08x, size=%d\n", (unsigned int)pSource, wCount);
	
	uint16_t wCsr = (bEnd << MGC_S_HSDMA_ENDPOINT) | (1 << MGC_S_HSDMA_ENABLE);
	uint32_t dwBoundary = 0x400 - ((uint32_t)pSource & 0x3ff);
	uint16_t wBurstSize = MGC_MIN((uint16_t)dwBoundary, 512);
	//bIdxTemp = reg_usb_Index;
	//reg_usb_Index = VIDEO_EP_INDEX;
	
	unsigned int	wIntrTxE = MGC_Read16(MGC_O_HDRC_INTRTXE);
	MGC_Write16(MGC_O_HDRC_INTRTXE, wIntrTxE & ~(1 << bEnd));
	
	
	old_TxCSRH = MGC_Read8(MUSB_IDX_TXCSRH);
	
	unsigned int TxCSRH = MGC_Read8(MUSB_IDX_TXCSRH);
	// D2 DMAReqMode 
	//	The CPU sets this bit to select DMA Request Mode 1 and clears it to select DMA Request Mode 0.
	TxCSRH |= TXCSRH_AutoSet|TXCSRH_DMAReqEnab|TXCSRH_DMAReqMode;
	MGC_Write8(MUSB_IDX_TXCSRH, TxCSRH);
		
	/* we always use the DMAC's mode 1 */
	wCsr |= 1 << MGC_S_HSDMA_MODE1;

	/* set burst mode */
	if(wBurstSize >= 64)
	{
		wCsr |= MGC_HSDMA_BURSTMODE_INCR16 << MGC_S_HSDMA_BURSTMODE;
	}
	else if(wBurstSize >= 32)
	{
		wCsr |= MGC_HSDMA_BURSTMODE_INCR8 << MGC_S_HSDMA_BURSTMODE;
	}
	else if(wBurstSize >= 16)
	{
		wCsr |= MGC_HSDMA_BURSTMODE_INCR4 << MGC_S_HSDMA_BURSTMODE;
	}
	wCsr |= 1 << MGC_S_HSDMA_TRANSMIT;
	wCsr |= 1 << MGC_S_HSDMA_IRQENABLE;
	
	// This register identifies the current memory address of the corresponding DMA channel. The Initial memory address written to
	// this register must have a value such that its modulo 4 value is equal to 0. That is, DMA_ADDR[1:0] must be equal to 2’b00. The
	// lower two bits of this register are read only and cannot be set by software. As the DMA transfer progresses, the memory address
	// will increment as bytes are tranfered.
	USB_DMA1ADDR = (unsigned int)pSource;

	// This register identifies the current DMA count of the transfer. Software will set the initial count of the transfer which identifies
	//	the entire transfer length. As the count progresses this count is decremented as bytes are transfered.
	USB_DMA1CNT = wCount;
	
	//printf ("usb_dma start USB_DMAINT=0x%x, wCount=%d\n", USB_DMAINT, wCount);
	
	unsigned int timeout_ticket = XM_GetTickCount () + 200;
	// When operating in DMA Mode 1, the DMA controller can be programmed to load/unload a complete bulk transfer (which can
	//	be many packets). Once set up, the DMA controller will load/unload all packets of the transfer, interrupting the processor only
	// when the transfer has completed. DMA Mode 1 can only be used with endpoints that use Bulk transactions.	
#if 0
	USB_DMA1CTL = 	(0 << 9)			// D10-D9 DMA_BRSTM 
											//		00 = Burst Mode 0 : Bursts of unspecified length
					|	(bEnd << 4)		// D7-D4 DMAEP The endpoint number this channel is assigned to.
					|	(1 << 3)			// D3 DMAIE DMA Interrupt Enable.	
					|	(1 << 2)			// D2 DMAMODE This bit selects the DMA Transfer Mode.
											//		0 = DMA Mode0 Transfer
											//		1 = DMA Mode1 Transfer
					|	(1 << 1)			// D1 DMA_DIR This bit selects the DMA Transfer Direction.
											//		0 = DMA Write (RX Endpoint)
											//		1 = DMa Read (TX Endpoint)
					|	(1 << 0)			// D0 DMA_ENAB This bit enables the DMA transfer and will cause the transfer to begin.	
					;	
#endif
	
	//if(wCount % 512)
	{
		//usb_dma_short_package = 1;
	}
	
	USB_DMA1CTL = wCsr;
	
	//printf ("usb_dma, USB_DMA1CNT=%d\n", USB_DMA1CNT);
	//return;
	
	while(1)
	{
		unsigned int dmaint = USB_DMAINT;
		if( dmaint & 0x01 )
		{
			printf ("dma int\n");
			break;
		}
		if(XM_GetTickCount() >= timeout_ticket)
			break;
	}
	
	//printf ("usb_dma end, USB_DMA1CNT=%d\n", USB_DMA1CNT);
	
	
	while(MGC_Read8(MUSB_IDX_TXCSRL) & TXCSRL_TxPktRdy)
	{
		if(XM_GetTickCount() >= timeout_ticket)
			break;
	}
	MGC_Write8 (MUSB_IDX_TXCSRH, old_TxCSRH);


	wIntrTxE = MGC_Read16(MGC_O_HDRC_INTRTXE);
	MGC_Write16(MGC_O_HDRC_INTRTXE, wIntrTxE | (1 << bEnd));
	
	void UVCTransfer (void);

	UVCTransfer ();
		
}

static void bulkLoadFifo(uint8_t bEnd, uint16_t wCount, const uint8_t* pSource)
{
	uint16_t wIndex;
	uint8_t bFifoOffset = MGC_FIFO_OFFSET(bEnd);

	unsigned int count = wCount;
	unsigned int burst_count;
	const uint8_t *data;
	volatile uint8_t*addr = (volatile uint8_t*)((uint8_t *)USB_BASE + bFifoOffset);
	data = pSource;
	burst_count = count & ~0x0F;
	
	if( (unsigned int)data & 3 )
	{
		printf ("pSource=0x%08x\n", (unsigned int)pSource);
		goto aligned_2;
	}
	
	burst_load_fifo (addr, (unsigned char *)data, burst_count);
	data += burst_count;
	count -= burst_count;
	/*while(count >= 16)
	{
		*(unsigned int *)addr = *(unsigned int *)data;
		*(unsigned int *)addr = *(unsigned int *)(data+4);
		*(unsigned int *)addr = *(unsigned int *)(data+8);
		*(unsigned int *)addr = *(unsigned int *)(data+12);
		data += 16;
		count -= 16;
	}*/
	while(count >= 8)
	{
		*(unsigned int *)addr = *(unsigned int *)data;
		*(unsigned int *)addr = *(unsigned int *)(data+4);
		data += 8;
		count -= 8;
	}

aligned_2:	
	if( (unsigned int)data & 1 )
		goto aligned_1;
	
	while(count > 2)
	{
		*(unsigned short *)addr = *(unsigned short *)data;
		data += 2;
		count -= 2;
	}
	
aligned_1:	
	while(count > 0)
	{
		*addr = *data ++;
		count --;
	}
}

static unsigned int uvc_output_image_cx = 0;
static unsigned int uvc_output_image_cy = 0;
void XMSYS_CameraGetWindowSize (XMSIZE *lpSize)
{
	int index = reg_jpegWinIdx;
	lpSize->cx = Frame_Width[index];
	lpSize->cy = Frame_Height[index];
		
	// 检查是否存在协议设置的输出图像尺寸设置. 若存在, 使用该输出尺寸设置
	if(uvc_output_image_cx && uvc_output_image_cy)
	{
		lpSize->cx = uvc_output_image_cx;
		lpSize->cy = uvc_output_image_cy;
	}
}

void XMSYS_CameraSetOutputImageSize (XMSIZE size)
{
	if(size.cx && size.cy)
	{
		uvc_output_image_cx = size.cx;
		uvc_output_image_cy = size.cy;
	}
}

void UVCStart (void)
{
	//printf ("UVCStart\n");
	XMSYS_CameraPostEvent (XMSYS_CAMERA_UVC_START);
}

// UVC Host触发协议终止
// 如Windows在不同的Camera直接切换
void UVCStop (void)
{
	//printf ("UVCStop\n");
	//uvc_dma_stop ();	
	UVCVideoReady = 0;
	XMSYS_CameraPostEvent (XMSYS_CAMERA_UVC_STOP);
}

// UVC异常终止(USB异常, suspend)
void UvcBreak (void)
{
	UVCVideoReady = 0;
	XMSYS_CameraPostEvent (XMSYS_CAMERA_UVC_BREAK);
	
}

extern void XMSYS_H264CodecForceIFrame (void);

struct UVCVIDEOSAMPLE * UVCVideoCreate (void)
{
	disable_usb_int();
	//printf ("UVCVideoCreate\n");
	//printf ("Window Size, Width=%d Height=%d\n", Frame_Width[reg_jpegWinIdx],  	Frame_Height[reg_jpegWinIdx]);
	memset (&UVCVideoSample, 0, sizeof(UVCVideoSample));
	// 设置ISO包的字节大小
	UVCVideoSample.dwPayloadDataSize = (USB20_VIDEO_DEPTH_H_VAL << 8) | USB20_VIDEO_DEPTH_L_VAL;	// VGA support
	printf ("UVCVideoCreate dwPayloadDataSize=%d\n", UVCVideoSample.dwPayloadDataSize);
	enable_usb_int();
	
	// 强制UVC创建时发送IFrame
	XMSYS_H264CodecForceIFrame ();
	
	return &UVCVideoSample;
}

void UVCVideoDelete (struct UVCVIDEOSAMPLE *lpVideoSample, int stop_or_break)
{
	unsigned char buf[PAYLOAD_HEADER_SIZE];
	unsigned int loop = 0;
	int i;
	BYTE BFH0;
	BYTE bIdxTemp;
	disable_usb_int();
	printf ("UVCVideoDelete\n");
	uvc_dma_stop ();
	UVCVideoReady = 0;
	// while(UVCVideoSample.cbPayloadData);
	
	// 20130504 ZhuoYongHong
	// 以下代码非常重要
	// 确保清除FIFO中上次UVC包尚未发送的包
	bIdxTemp = reg_usb_Index;
	reg_usb_Index = VIDEO_EP_INDEX;
	while(MGC_Read8(MUSB_IDX_TXCSRL) & TXCSRL_FIFONotEmpty)
	{
		loop ++;
		if(loop >= 0x100000)
			break;
		// printf ("FlushFIFO\n");
		// MGC_Write8(MUSB_IDX_TXCSRL, TXCSRL_TxPktRdy);	
		MGC_Write8(MUSB_IDX_TXCSRL, TXCSRL_FlushFIFO) ;
	}
	
	if(stop_or_break == 1)
	{
		// 协议终止
		// 插入Payload结束包
		BFH0 = lpVideoSample->FID;
		BFH0 |= UVC_STREAM_EOF|UVC_STREAM_ERR;
		buf[0] = PAYLOAD_HEADER_SIZE;
		buf[1] = BFH0;
		buf[2] = 0;
		buf[3] = 0;
		bulkLoadFifo(VIDEO_EP_INDEX, PAYLOAD_HEADER_SIZE, buf);
		memset (buf, 0, sizeof(buf));
		for (i = 0; i < 512/PAYLOAD_HEADER_SIZE; i++)
		{
			bulkLoadFifo(VIDEO_EP_INDEX, PAYLOAD_HEADER_SIZE, buf);
		}
		MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_TxPktRdy|TXCSRL_ClrDataTog) ;
	}
	
	reg_usb_Index = bIdxTemp;
	
	memset (&UVCVideoSample, 0, sizeof(UVCVideoSample));

	enable_usb_int();
}

//  *********** 针对Bulk In 端口 5 ******************
//
// 1) individual Payload 指USB端口定义的包字节大小，either 8, 16, 32, 64 or 512 bytes in size。
//		512仅定义在High Speed模式
// 2) Packet Size 定义在TxMaxP寄存器，可以为一个或多个single individual Payload组成
// 3) The sizes of the Tx FIFOs (端口 5)已在芯片设计时固定为4KB
//
// 当Packet Size小于或等于1KB时(端口 5的FIFO 2KB 一半大小), double packet buffering is enabled。
//

// 准备并开始UVC Sample传输
void UVCVideoSetupPayload (BYTE *lpPayload, DWORD cbPayload)
{	
	// printf ("UVCVideoSetupPayload cbPayload=%d\n", cbPayload);
	BYTE TxCSRL, TxCSRH;
	DWORD mod;
	BYTE bIdxTemp;
	
	// 复位后 WiFi路由器不能传输
	//printf ("UVCVideoSetupPayload\n");
	
	
	//printf ("UVCVideoSetupPayload cbPayload\n" );
	
	if(cbPayload == 0)
		return;

	disable_usb_int();
	bIdxTemp = reg_usb_Index;
	reg_usb_Index = VIDEO_EP_INDEX;
	// 检查是否FIFO非空
	/*
	while(1)
	{
		TxCSRL = MGC_Read8(MUSB_IDX_TXCSRL);
		if((TxCSRL & TXCSRL_FIFONotEmpty))
		{
			printf ("TXCSRL_FIFONotEmpty\n");
			OS_Delay (1);
		}
		else
			break;
	}*/

	// 清空FIFO
	//MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_FlushFIFO) ;

#if 0//_PAYLOAD_PIPE_ == 	_PAYLOAD_PIPE_ISO_
	cbPayload = cbPayload + UVCVideoSample.dwPacketSize - 2 - 1;
	cbPayload = cbPayload / (UVCVideoSample.dwPacketSize - 2);
	cbPayload *= (UVCVideoSample.dwPacketSize - 2);
#endif
	
	mod = cbPayload % (UVCVideoSample.dwPayloadDataSize - PAYLOAD_HEADER_SIZE);
	mod %= 512;	// Bulk的包长度为512
	if(mod == 0)
	{
		// printf ("Bulk 0 Packet avoid\n");
		cbPayload ++;	// 避免0包
	}
	
	
#if _VIDEO_EP_ == _VIDEO_EP_4_
	// 512B
	mod = (cbPayload + (UVCVideoSample.dwPacketSize - PAYLOAD_HEADER_SIZE) - 1) / (UVCVideoSample.dwPacketSize - PAYLOAD_HEADER_SIZE);
	if(mod & 1)
	{
		// 奇怪Windows UVC下，必须补齐为偶数个帧，这样第一个帧就不会丢失
		// 奇数包
		// printf ("even packet\n");
		
		cbPayload += (UVCVideoSample.dwPacketSize - PAYLOAD_HEADER_SIZE);
	}
#elif _VIDEO_EP_ == _VIDEO_EP_5_
	// 1024 Payload
	// 测试补齐为偶数个512包
	// 参考UVC SPEC USB Device Class Definition for Video Devices: MJPEG Payloa
	/// Figure 4-3 Example MJPEG Bulk Transfer, IN Endpoint, 
	// 最后一个Video Sample必须包含一个DATA0 + DATA1
	
	// 计算最后一个Video Sample的Payload Size
	mod = cbPayload % (UVCVideoSample.dwPayloadDataSize - PAYLOAD_HEADER_SIZE);
	if( (mod + PAYLOAD_HEADER_SIZE) <= 512 )	// 
	{
		// 最后一个Video Sample必须为DATA0 + DATA1
		cbPayload += (513 - mod - PAYLOAD_HEADER_SIZE);
	}	
#endif
	
	
	//cbPayload = cbPayload + 1024;
		
	struct UVCVIDEOSAMPLE *lpVideoSample = &UVCVideoSample;		
	if(lpVideoSample->FID)
		lpVideoSample->FID = 0;
	else
		lpVideoSample->FID = UVC_STREAM_FID;
	
	lpVideoSample->lpVideoSample = lpPayload;
	lpVideoSample->cbVideoSample = cbPayload;
	lpVideoSample->bHeadLength = PAYLOAD_HEADER_SIZE;
	lpVideoSample->lpPayloadData = lpPayload;
	lpVideoSample->cbPayloadData = cbPayload;
	
	UVCVideoReady = 1;
	
#if _PAYLOAD_PIPE_ == _PAYLOAD_PIPE_BULK_
	// Force DATA0
	// MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_ClrDataTog) ;
#endif
	
	// UVC Payload传输改为非中断方式
	UVCVideoTransferPayload ();
	
	
	//MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_TxPktRdy|TXCSRL_ClrDataTog) ;
	
	reg_usb_Index = bIdxTemp;
	enable_usb_int();	
}

// UVC Payload传输改为非中断方式
int UVCVideoTransferPayload (void)
{
	BYTE *buff;
	DWORD size;
	unsigned char buf[PAYLOAD_HEADER_SIZE];
	BYTE BFH0;
	BYTE bIdxTemp;
	unsigned int burst_size;
	struct UVCVIDEOSAMPLE *lpVideoSample = &UVCVideoSample;

	if(UVCVideoReady == 0)
	{
		//XMSYS_CameraNotifyDataFrameTransferDone ();
		return 0;
	}

	if(lpVideoSample->cbPayloadData == 0)
	{
		//OS_Delay (10);
		UVCVideoReady = 0;
		XMSYS_CameraNotifyDataFrameTransferDone ();
		
		return 0;
	}
	
	bIdxTemp = reg_usb_Index;
	
	// BULK传输
	BFH0 = lpVideoSample->FID;
	MGC_SelectEnd(VIDEO_EP_INDEX);
	
	while(MGC_Read8(MUSB_IDX_TXCSRL) & TXCSRL_TxPktRdy);
	
	size = lpVideoSample->dwPayloadDataSize - PAYLOAD_HEADER_SIZE;
	if(size > lpVideoSample->cbPayloadData)
	{
		size = lpVideoSample->cbPayloadData;
		// 最后一个Payload
		BFH0 |= UVC_STREAM_EOF;
	}
	
	uint8_t bFifoOffset = MGC_FIFO_OFFSET(VIDEO_EP_INDEX);
	volatile uint8_t*addr = (volatile uint8_t*)((uint8_t *)USB_BASE + bFifoOffset);
	*(unsigned int *)addr = PAYLOAD_HEADER_SIZE | (BFH0 << 8);
	
	// 1024 payload size
	// 传输Payload
	buff = (BYTE *)lpVideoSample->lpPayloadData;
	
	bulkLoadFifo(VIDEO_EP_INDEX, size, buff);	
	MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_TxPktRdy) ;
	
	//bulkLoadFifo_payload (VIDEO_EP_INDEX, size, buff);	
	lpVideoSample->lpPayloadData += size;
	lpVideoSample->cbPayloadData -= size;
	
	reg_usb_Index = bIdxTemp;
		
	return size;
}

void UVCVideoStallVideoIn (void)
{
#if _PAYLOAD_PIPE_ == _PAYLOAD_PIPE_BULK_
	// Stall EP
	BYTE bIdxTemp;
	disable_usb_int();
	bIdxTemp = reg_usb_Index;
	reg_usb_Index = VIDEO_EP_INDEX;
	MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_SendStall) ;
	reg_usb_Index = bIdxTemp;
	enable_usb_int();
#endif
}

void UVCVideoResume (void)
{
	
}

//#define	UVC_TRANSFER_BY_INTERRUPT
//	使用中断方式传输容易导致ISP Scalar 出现Y/UV Pop Error 

void UVCTransfer (void)
{
#ifdef UVC_TRANSFER_BY_INTERRUPT
	// 中断传递
	UVCVideoTransfer ();
#else
	// 任务传递
	XMSYS_CameraPostEvent (XMSYS_CAMERA_UVC_TRANSFER);
#endif
}

#ifdef UVC_PIO
void UVCVideoTransfer (void)
{
	BYTE bIdxTemp;
#ifndef UVC_TRANSFER_BY_INTERRUPT
	disable_usb_int();
#endif
	//bIdxTemp = reg_usb_Index;
	reg_usb_Index = VIDEO_EP_INDEX;
	UVCVideoTransferPayload();
	//reg_usb_Index = bIdxTemp;
#ifndef UVC_TRANSFER_BY_INTERRUPT
	enable_usb_int();
#endif
}

#else

void UVCVideoTransfer (void)
{
	//BYTE bIdxTemp;
#ifndef UVC_TRANSFER_BY_INTERRUPT
	disable_usb_int();
#endif
	//bIdxTemp = reg_usb_Index;
	//reg_usb_Index = VIDEO_EP_INDEX;
	UVC_VideoTransferPayload(&UVCVideoSample);
	//reg_usb_Index = bIdxTemp;
#ifndef UVC_TRANSFER_BY_INTERRUPT
	enable_usb_int();
#endif
}

#endif

// 配置当前的payload参数 
// 返回值
//		0			UVC传输完毕
//		1			payload配置成功
static int UVC_VideoSetupPayload (struct UVCVIDEOSAMPLE *VideoSample)
{
	BYTE *lpPayload;
	DWORD cbPayload;
	DWORD size;
	DWORD mod;
	
	if(VideoSample == NULL)
		return 0;
	
	lpPayload = (BYTE *)VideoSample->lpVideoSample;
	cbPayload = VideoSample->cbVideoSample;
	if(cbPayload == 0)
		return 0;
	
	// 计算payload data size
	if( (cbPayload + PAYLOAD_HEADER_SIZE) > VideoSample->dwPayloadDataSize )
		cbPayload = VideoSample->dwPayloadDataSize - PAYLOAD_HEADER_SIZE;
	
	//printf ("UVC_VideoSetupPayload cbPayload=%d\n", cbPayload);
	
	VideoSample->cbVideoSample -= cbPayload;
	VideoSample->lpVideoSample += cbPayload;
	
	// 参考UVC SPEC USB Device Class Definition for Video Devices: MJPEG Payloa
	/// Figure 4-3 Example MJPEG Bulk Transfer, IN Endpoint, 
	// 最后一个Video Sample必须包含一个DATA0 + DATA1
	if( (cbPayload + PAYLOAD_HEADER_SIZE) <= 512 )
		cbPayload = 513 - PAYLOAD_HEADER_SIZE;		// DATA0 + DATA1

	VideoSample->lpPayloadData = lpPayload;
	VideoSample->cbPayloadData = cbPayload;
	VideoSample->cbPayloadSize = cbPayload;
	return 1;
}

static void UVC_VideoDmaTransfer (struct UVCVIDEOSAMPLE *VideoSample, uint8_t bEnd, uint16_t wCount, const uint8_t* pSource)
{
	//unsigned int old_TxCSRH;
	//unsigned int bIdxTemp;
	
	//printf ("usb dma 0x%08x, size=%d\n", (unsigned int)pSource, wCount);
	
	uint16_t wCsr = (bEnd << MGC_S_HSDMA_ENDPOINT) | (1 << MGC_S_HSDMA_ENABLE);
	uint32_t dwBoundary = 0x400 - ((uint32_t)pSource & 0x3ff);
	uint16_t wBurstSize = MGC_MIN((uint16_t)dwBoundary, 512);
	
	//old_TxCSRH = MGC_Read8(MUSB_IDX_TXCSRH);
	
	unsigned int TxCSRH = MGC_Read8(MUSB_IDX_TXCSRH);
	// D2 DMAReqMode 
	//	The CPU sets this bit to select DMA Request Mode 1 and clears it to select DMA Request Mode 0.
	TxCSRH |= TXCSRH_AutoSet|TXCSRH_DMAReqEnab|TXCSRH_DMAReqMode;
	MGC_Write8(MUSB_IDX_TXCSRH, TxCSRH);
		
	/* we always use the DMAC's mode 1 */
	wCsr |= 1 << MGC_S_HSDMA_MODE1;

	/* set burst mode */
	if(wBurstSize >= 64)
	{
		wCsr |= MGC_HSDMA_BURSTMODE_INCR16 << MGC_S_HSDMA_BURSTMODE;
	}
	else if(wBurstSize >= 32)
	{
		wCsr |= MGC_HSDMA_BURSTMODE_INCR8 << MGC_S_HSDMA_BURSTMODE;
	}
	else if(wBurstSize >= 16)
	{
		wCsr |= MGC_HSDMA_BURSTMODE_INCR4 << MGC_S_HSDMA_BURSTMODE;
	}
	wCsr |= 1 << MGC_S_HSDMA_TRANSMIT;
	wCsr |= 1 << MGC_S_HSDMA_IRQENABLE;
	
	// This register identifies the current memory address of the corresponding DMA channel. The Initial memory address written to
	// this register must have a value such that its modulo 4 value is equal to 0. That is, DMA_ADDR[1:0] must be equal to 2’b00. The
	// lower two bits of this register are read only and cannot be set by software. As the DMA transfer progresses, the memory address
	// will increment as bytes are tranfered.
	USB_DMA1ADDR = (unsigned int)pSource;

	// This register identifies the current DMA count of the transfer. Software will set the initial count of the transfer which identifies
	//	the entire transfer length. As the count progresses this count is decremented as bytes are transfered.
	USB_DMA1CNT = wCount;
	
	//printf ("usb_dma start USB_DMAINT=0x%x, wCount=%d\n", USB_DMAINT, wCount);
	//VideoSample->dma_timeout = XM_GetTickCount () + 200;
	VideoSample->dma_timeout = XM_GetTickCount () + 300;		// 测试中发现存在超时, 增加超时时限200->300
	VideoSample->dma_transfer_start = 1;
	// When operating in DMA Mode 1, the DMA controller can be programmed to load/unload a complete bulk transfer (which can
	//	be many packets). Once set up, the DMA controller will load/unload all packets of the transfer, interrupting the processor only
	// when the transfer has completed. DMA Mode 1 can only be used with endpoints that use Bulk transactions.	
#if 0
	USB_DMA1CTL = 	(0 << 9)			// D10-D9 DMA_BRSTM 
											//		00 = Burst Mode 0 : Bursts of unspecified length
					|	(bEnd << 4)		// D7-D4 DMAEP The endpoint number this channel is assigned to.
					|	(1 << 3)			// D3 DMAIE DMA Interrupt Enable.	
					|	(1 << 2)			// D2 DMAMODE This bit selects the DMA Transfer Mode.
											//		0 = DMA Mode0 Transfer
											//		1 = DMA Mode1 Transfer
					|	(1 << 1)			// D1 DMA_DIR This bit selects the DMA Transfer Direction.
											//		0 = DMA Write (RX Endpoint)
											//		1 = DMa Read (TX Endpoint)
					|	(1 << 0)			// D0 DMA_ENAB This bit enables the DMA transfer and will cause the transfer to begin.	
					;	
#endif
	
	
	USB_DMA1CTL = wCsr;
		
}

int UVC_VideoTransferPayload (struct UVCVIDEOSAMPLE *VideoSample)
{
	BYTE *buff;
	uint8_t bFifoOffset;
	volatile uint8_t*addr;
	DWORD to_transfer;
	BYTE BFH0;
	int ret = 0;
	BYTE bIdxTemp;
		
	do
	{
		if(UVCVideoReady == 0)
			break;
		
		if(VideoSample == NULL)
			break;
		
		if(VideoSample->cbPayloadData == 0)
		{
			if(UVC_VideoSetupPayload(VideoSample) == 0)
			{
				// UVC sample data 传输完毕
				UVCVideoReady = 0;
				XMSYS_CameraNotifyDataFrameTransferDone ();
				// UVC transfer finish
				break;
			}
		}
		
		if(USB_DMA1CTL & (1 << MGC_S_HSDMA_BUSERROR))
		{
			// Bus Error Bit. Indicates that a bus error has been observed on the input
			//		AHB_HRESPM[1:0]. This bit is cleared by software.
			printf ("HSDMA_BUSERROR\n");
			USB_DMA1CTL &= ~(1 << MGC_S_HSDMA_BUSERROR);
			//UVCVideoReady = 0;
			//XMSYS_CameraNotifyDataFrameTransferDone ();
			//break;
		}
		
		bIdxTemp = reg_usb_Index;
		MGC_SelectEnd (VIDEO_EP_INDEX);
		
		MGC_Write8(MUSB_IDX_TXCSRL, 0);
		MGC_Write8(MUSB_IDX_TXCSRH, TXCSRH_Mode) ;
		
		BFH0 = VideoSample->FID;
		// assert ( (VideoSample->cbPayloadData + PAYLOAD_HEADER_SIZE) > 512 );
		// 检查是否是包含header信息的传输阶段
		to_transfer = 0;
		if(VideoSample->cbPayloadData == VideoSample->cbPayloadSize)
		{
			//printf ("pio 1\n");
			// 20170314 实测中以下代码会导致DMA timeout
			//unsigned int loop = 0;
			//while( MGC_Read8(MUSB_IDX_TXCSRL) & TXCSRL_TxPktRdy )
			/*
			while( MGC_Read8(MUSB_IDX_TXCSRL) & TXCSRL_FIFONotEmpty )
			{
				loop ++;
				if(loop >= 0x100000)
					break;
				MGC_Write8(MUSB_IDX_TXCSRL, TXCSRL_FlushFIFO) ;
			}*/
			
			//MGC_Write8(MUSB_IDX_TXCSRL, TXCSRL_ClrDataTog);
			//MGC_Write8(MUSB_IDX_TXCSRL, 0);
			// 包含header信息的传输阶段
			if( (VideoSample->cbPayloadData + PAYLOAD_HEADER_SIZE) < (512 * 2) )
			{
				// 最后一个Video Sample必须为DATA0 + DATA1
				BFH0 |= UVC_STREAM_EOF;
			}
			// 使用PIO传输DATA0
			bFifoOffset = MGC_FIFO_OFFSET(VIDEO_EP_INDEX);
			addr = (volatile uint8_t*)((uint8_t *)USB_BASE + bFifoOffset);
			*(volatile unsigned int *)addr = PAYLOAD_HEADER_SIZE | (BFH0 << 8);
			to_transfer = 512 - PAYLOAD_HEADER_SIZE;
			bulkLoadFifo (VIDEO_EP_INDEX, to_transfer, (unsigned char *)VideoSample->lpPayloadData);
			MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_TxPktRdy);
			//MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_TxPktRdy|TXCSRL_ClrDataTog) ;		// DATA0
		}
		else// if(VideoSample->cbPayloadData)
		{
			// payload data的传输阶段
			to_transfer = VideoSample->cbPayloadData;
			// 检查待传数据字节长度是否满足DMA传输 (512字节)
			if(to_transfer >= 512)
			{
				// DMA传输
				//printf ("dma\n");
				to_transfer = to_transfer & ~511;
				UVC_VideoDmaTransfer (VideoSample, VIDEO_EP_INDEX, to_transfer, (unsigned char *)VideoSample->lpPayloadData);
				//to_transfer = 512;
				//bulkLoadFifo (VIDEO_EP_INDEX, to_transfer, (unsigned char *)VideoSample->lpPayloadData);
				//MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_TxPktRdy) ;
			}
			else
			{
				// 不够512字节长度的数据使用PIO传输
				//printf ("pio 2\n");
				bulkLoadFifo (VIDEO_EP_INDEX, to_transfer, (unsigned char *)VideoSample->lpPayloadData);
				MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_TxPktRdy) ;
			}
		}
		//printf ("cbPayloadData=%d, to_trans=%d\n", VideoSample->cbPayloadData, to_transfer);
		VideoSample->lpPayloadData += to_transfer;
		VideoSample->cbPayloadData -= to_transfer;
		
		ret = 1;
		
		reg_usb_Index = bIdxTemp;
		
	} while (0);
		
	return ret;
}

// 准备并开始UVC Sample传输
void UVC_VideoSetupVideoSample (BYTE *lpVideoSample, DWORD cbVideoSample)
{	
	struct UVCVIDEOSAMPLE *VideoSample = &UVCVideoSample;
	//printf ("UVC_VideoSetupVideoSample cbVideoSample=%d\n", cbVideoSample);
	if(cbVideoSample == 0 || lpVideoSample == NULL)
		return;
	
	disable_usb_int();
	if(VideoSample->FID)
		VideoSample->FID = 0;
	else
		VideoSample->FID = UVC_STREAM_FID;
	
	VideoSample->lpVideoSample = lpVideoSample;
	VideoSample->cbVideoSample = cbVideoSample;
	VideoSample->bHeadLength = PAYLOAD_HEADER_SIZE;
	VideoSample->lpPayloadData = 0;
	VideoSample->cbPayloadData = 0;
	VideoSample->cbPayloadSize = 0;
	
	UVCVideoReady = 1;
	
	UVC_VideoTransferPayload (VideoSample);
	enable_usb_int();	
	
}

void uvc_dma_stop (void)
{
	disable_usb_int();
	if(UVCVideoSample.dma_transfer_start)
	{
		unsigned int loop = 0;
		USB_DMA1CTL = 0;
		
		delay (100);
					
		//MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_TxPktRdy) ;
		//MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_FlushFIFO) ;					
				
		while( MGC_Read8(MUSB_IDX_TXCSRL) & TXCSRL_FIFONotEmpty )
		{
			loop ++;
			if(loop >= 0x100000)
				break;
			MGC_Write8(MUSB_IDX_TXCSRL, TXCSRL_FlushFIFO) ;
		}
		MGC_Write8(MUSB_IDX_TXCSRL, TXCSRL_ClrDataTog) ;
				
		UVCVideoSample.dma_transfer_start = 0;
		UVCVideoSample.cbVideoSample = 0;
		UVCVideoSample.cbPayloadData = 0;
		UVCVideoSample.cbPayloadSize = 0;
	}
	enable_usb_int();	
}

void uvc_usb_dma_transfer_finish_interrupt (void)
{	
	unsigned int dma_int = USB_DMAINT;
	if(dma_int & 0x01)
	{
		//printf ("dma end\n");
		UVCVideoSample.dma_transfer_start = 0;
		UVCTransfer ();
	}
}
