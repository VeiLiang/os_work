#include <xm_proj_define.h>
#include <string.h>
#include "common.h"
#include "gpio_function.h"
#include "sensorlib.h"
#include "SensorCommon.h"
#include <stdio.h>




#if MASK_VER_2
BYTE code Fs_AltSet[12] =
{
	0x20,	// 320X240/176X144/160X120窗口在USB1.0下的带宽需求 - USB11_INMAX_L_VIDEO_VAL
	0x02,	// 320X240/176X144/160X120窗口在USB1.0下的带宽需求 - USB11_INMAX_H_VIDEO_VAL
	0x20,	// 320X240/176X144/160X120窗口在USB1.0下的带宽需求 - USB11_VIDEO_DEPTH_L_VAL
	0x02,	// 320X240/176X144/160X120窗口在USB1.0下的带宽需求 - USB11_VIDEO_DEPTH_H_VAL

	0x20,	// 352X288窗口在USB1.0下的带宽需求 - USB11_INMAX_L_VIDEO_VAL
	0x02,	// 352X288窗口在USB1.0下的带宽需求 - USB11_INMAX_H_VIDEO_VAL
	0x20,	// 352X288窗口在USB1.0下的带宽需求 - USB11_VIDEO_DEPTH_L_VAL
	0x02,	// 352X288窗口在USB1.0下的带宽需求 - USB11_VIDEO_DEPTH_H_VAL

	0x38,	// 其他窗口(>352x288)在USB1.0下的带宽需求 - USB11_INMAX_L_VIDEO_VAL
	0x03,	// 其他窗口(>352x288)在USB1.0下的带宽需求 - USB11_INMAX_H_VIDEO_VAL
	0x38,	// 其他窗口(>352x288)在USB1.0下的带宽需求 - USB11_VIDEO_DEPTH_L_VAL
	0x03,	// 其他窗口(>352x288)在USB1.0下的带宽需求 - USB11_VIDEO_DEPTH_H_VAL
};

BYTE code * xdata pFs_AltSet_addr;

BYTE code Hs_AltSet[8] =
{
	0x80,	// 176x144/160x120窗口在USB2.0下的带宽需求 - USB20_INMAX_L_VIDEO_VAL
	0x0a,	// 176x144/160x120窗口在USB2.0下的带宽需求 - USB20_INMAX_H_VIDEO_VAL
	0x00,	// 176x144/160x120窗口在USB2.0下的带宽需求 - USB20_VIDEO_DEPTH_L_VAL
	0x05,	// 176x144/160x120窗口在USB2.0下的带宽需求 - USB20_VIDEO_DEPTH_H_VAL

	// USB 2.0高速模式下 Max Packet Size 及 Payload Size定义

#if _PAYLOAD_PIPE_ == _PAYLOAD_PIPE_ISO_	
	// Packet Size定义
	0x00,	// 其他窗口在USB2.0下的带宽需求 - USB11_INMAX_L_VIDEO_VAL
	0x04,	// 其他窗口在USB2.0下的带宽需求 - USB11_INMAX_H_VIDEO_VAL
	
	// Payload Size定义
	0xb8,	// 其他窗口在USB2.0下的带宽需求 - USB11_VIDEO_DEPTH_L_VAL
//	0x0b,	// 其他窗口在USB2.0下的带宽需求 - USB11_VIDEO_DEPTH_H_VAL
	0x03,	// 其他窗口在USB2.0下的带宽需求 - USB11_VIDEO_DEPTH_H_VAL
#elif _PAYLOAD_PIPE_ == _PAYLOAD_PIPE_BULK_
	// Packet Size定义, 512 for Bulk In Ep
	0x00,	// 其他窗口在USB2.0下的带宽需求 - USB11_INMAX_L_VIDEO_VAL
	0x02,	// 其他窗口在USB2.0下的带宽需求 - USB11_INMAX_H_VIDEO_VAL

#if _VIDEO_EP_ == _VIDEO_EP_4_
	// Payload Size定义,512
	0x00,	// 其他窗口在USB2.0下的带宽需求 - USB11_VIDEO_DEPTH_L_VAL
	0x02,	// 其他窗口在USB2.0下的带宽需求 - USB11_VIDEO_DEPTH_H_VAL
#elif _VIDEO_EP_ == _VIDEO_EP_5_
	// Payload Size定义,1024
	0x00, // 其他窗口在USB2.0下的带宽需求 - USB11_VIDEO_DEPTH_L_VAL
#ifdef UVC_PIO	
	0x04, // 其他窗口在USB2.0下的带宽需求 - USB11_VIDEO_DEPTH_H_VAL
#else
					// Payload值越大, UVC帧率与H264编码的帧率越接近.
					// Payload值越小, UVC帧率越小, H264帧率越高. 当Payload小于0x800时,H264的帧率固定为30帧, UVC帧率<16帧
	//0x80,		// 测试用于大块Payload data
	//0x20		// 380MHz DDR, H264编码帧率为27帧, UVC帧率(720P)为(26~28)帧左右	
	//0x10		// 380MHz DDR, 基本与0x20一致
	//0x0E			// 380MHz DDR, 
	//0x0c		// 380MHz DDR, H264编码帧率为28.5, UVC帧率(720P)为(18~24)帧左右
	0x08,		// 测试用于大块Payload data, 值太小, UVC帧率无法保证, 此时H264的帧率固定为30帧
					//		380MHz DDR, H264编码帧率为30帧, UVC帧率(720P)为16帧左右
					// 使用较小的Payload, 对ISP的影响将为较小
#endif
#endif
#endif	
};
BYTE code * xdata pHs_AltSet_addr;
#endif


void set_dsp_default()
{
	//确保属性值为默认值，默认值在前面已保存成当前值
	reg_brightness_w (gsVideoStatus.bBrightness);	//gsVideoDefault.bBrightness;
	reg_contrast_w ( gsVideoStatus.bContrast);	//gsVideoDefault.bContrast;
	reg_saturation_w ( gsVideoStatus.bSaturation);//gsVideoDefault.bSaturation;
	reg_hue_w  (gsVideoStatus.wHue);	//gsVideoDefault.wHue;
}


// 320X240/176X144/160X120窗口在USB1.0下的带宽需求
void Sensor_FsQvga_AltSet()
{
	
	//pFs_AltSet_addr = Fs_AltSet;	//在Sensor_Common_Sel_AltSet()执行前指定地址

	USB11_INMAX_L_VIDEO_VAL = *(pFs_AltSet_addr+0);
	USB11_INMAX_H_VIDEO_VAL = *(pFs_AltSet_addr+1);
	USB11_VIDEO_DEPTH_L_VAL = *(pFs_AltSet_addr+2);
	USB11_VIDEO_DEPTH_H_VAL = *(pFs_AltSet_addr+3);
}

// 352X288窗口在USB1.0下的带宽需求
void Sensor_FsCif_AltSet()
{
	
	//pFs_AltSet_addr = Fs_AltSet;	//在Sensor_Common_Sel_AltSet()执行前指定地址

	USB11_INMAX_L_VIDEO_VAL = *(pFs_AltSet_addr+4);
	USB11_INMAX_H_VIDEO_VAL = *(pFs_AltSet_addr+5);
	USB11_VIDEO_DEPTH_L_VAL = *(pFs_AltSet_addr+6);
	USB11_VIDEO_DEPTH_H_VAL = *(pFs_AltSet_addr+7);
}

// 其他窗口(>352x288)在USB1.0下的带宽需求，受JPEG压缩的图像质量所限，不能再支持2个设备同时播放
void Sensor_FsOtherWin_AltSet()
{
	
	//pFs_AltSet_addr = Fs_AltSet;	//在Sensor_Common_Sel_AltSet()执行前指定地址

	USB11_INMAX_L_VIDEO_VAL = *(pFs_AltSet_addr+8);
	USB11_INMAX_H_VIDEO_VAL = *(pFs_AltSet_addr+9);
	USB11_VIDEO_DEPTH_L_VAL = *(pFs_AltSet_addr+10);
	USB11_VIDEO_DEPTH_H_VAL = *(pFs_AltSet_addr+11);
}

// 176x144/160x120窗口在USB2.0下的带宽需求
void Sensor_HsQcif_AltSet()
{
	
	//pHs_AltSet_addr = Hs_AltSet;	//在Sensor_Common_Sel_AltSet()执行前指定地址

	USB20_INMAX_L_VIDEO_VAL = *(pHs_AltSet_addr+0);
	USB20_INMAX_H_VIDEO_VAL = *(pHs_AltSet_addr+1);
	USB20_VIDEO_DEPTH_L_VAL = *(pHs_AltSet_addr+2);
	USB20_VIDEO_DEPTH_H_VAL = *(pHs_AltSet_addr+3);
}

// 其他窗口在USB2.0下的带宽需求
void Sensor_HsOtherWin_AltSet()
{
	
	//pHs_AltSet_addr = Hs_AltSet;	//在Sensor_Common_Sel_AltSet()执行前指定地址

	USB20_INMAX_L_VIDEO_VAL = *(pHs_AltSet_addr+4);
	USB20_INMAX_H_VIDEO_VAL = *(pHs_AltSet_addr+5);
	USB20_VIDEO_DEPTH_L_VAL = *(pHs_AltSet_addr+6);
	USB20_VIDEO_DEPTH_H_VAL = *(pHs_AltSet_addr+7);

	// printf ("USB20_INMAX_VIDEO_VAL=%02x%02x\n", USB20_INMAX_H_VIDEO_VAL, USB20_INMAX_L_VIDEO_VAL);

	// printf ("USB20_VIDEO_DEPTH_VAL=%02x%02x\n", USB20_VIDEO_DEPTH_H_VAL, USB20_VIDEO_DEPTH_L_VAL);
}

void get_Jpeg_CurJpegSize(BYTE bFormat)		//1 - yuv, 0 - mjpeg
{

}

//如某些窗口需要更改带宽设置，则此函数必须要做外挂
//根据不同窗口更改如下几个变量值
// USB11_INMAX_L_VIDEO_VAL & USB11_INMAX_H_VIDEO_VAL - 影响reg_usb_InMaxPL & reg_usb_InMaxPH 以及USB11_MAX_VIDEO_PAYLOAD_SIZE
// USB11_VIDEO_DEPTH_L_VAL & USB11_VIDEO_DEPTH_H_VAL - 影响video_depth_1frameL & video_depth_1frameH
// USB20_INMAX_L_VIDEO_VAL & USB20_INMAX_H_VIDEO_VAL - 影响reg_usb_InMaxPL & reg_usb_InMaxPH 以及USB20_MAX_VIDEO_PAYLOAD_SIZE
// USB20_VIDEO_DEPTH_L_VAL & USB20_VIDEO_DEPTH_H_VAL - 影响video_depth_1frameL & video_depth_1frameH
void Sensor_Common_Sel_AltSet()
{
	BYTE idata bIdxTemp;

	if(gsDeviceStatus.bUsb_HighSpeed == 0)			  //fs
	{
		if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex -1] == 0x0a) 	// 160x120
		{
			Sensor_FsQvga_AltSet();
		}
		else if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex -1] == 0x0b) 	// 176x144
		{
			Sensor_FsQvga_AltSet();
		}
		else if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex -1] == 0x14) 	// 320x240
		{
			Sensor_FsQvga_AltSet();
		}
		else if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex -1] == 0x16)	//352x288
		{
			Sensor_FsCif_AltSet();
		}
		else if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex -1] >= 0x16)	// >= 640x480
		{
			Sensor_FsOtherWin_AltSet();

			get_Jpeg_CurJpegSize(0);
		}
	

		bIdxTemp = reg_usb_Index;
		reg_usb_Index = VIDEO_EP_INDEX;
		
		MGC_Write16(MUSB_IDX_TXMAXP, (USB11_INMAX_H_VIDEO_VAL << 8)|USB11_INMAX_L_VIDEO_VAL) ;
		reg_usb_Index = bIdxTemp;

		USB11_MAX_VIDEO_PAYLOAD_SIZE=(((DWORD)USB11_INMAX_L_VIDEO_VAL)<<24)+(((DWORD)USB11_INMAX_H_VIDEO_VAL)<<16);	
		gsVideoProbeCtrl.dwMaxPayloadTransferSize = USB11_MAX_VIDEO_PAYLOAD_SIZE;	//for MAC

		if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex-1] == 0x28)
		{
			gsVideoProbeCtrl.dwMaxVideoFrameSize = 0x00b00400;//0x00b00400;
		}
		else
		{
			gsVideoProbeCtrl.dwMaxVideoFrameSize = mjpeg_winFrame_BufferSize[gsVideoProbeCtrl.bFrameIndex-1];
		}
	}
	else		//hs
	{
		if(bFlg_mjpeg == 0)		//hs yuv
		{
			if(yuv_winFrame_order[gsVideoProbeCtrl.bFrameIndex -1] == 0x0a)		//160x120
			{
				Sensor_HsQcif_AltSet();
			}
			else if(yuv_winFrame_order[gsVideoProbeCtrl.bFrameIndex -1] == 0x0b)		//176x144
			{
				Sensor_HsQcif_AltSet();
			}
			else if(yuv_winFrame_order[gsVideoProbeCtrl.bFrameIndex -1] == 0x14)		//320x240
			{
			 	Sensor_HsOtherWin_AltSet();
			}
			else if(yuv_winFrame_order[gsVideoProbeCtrl.bFrameIndex -1] == 0x16)		//352x288
			{
			 	Sensor_HsOtherWin_AltSet();
			}
			//else if(yuv_winFrame_order[gsVideoProbeCtrl.bFrameIndex -1] == 0x28)		//640x480
			else if(yuv_winFrame_order[gsVideoProbeCtrl.bFrameIndex -1] >= 0x16)	// >= 640x480
			{
			 	Sensor_HsOtherWin_AltSet();

				get_Jpeg_CurJpegSize(1);
			}
		}
		else  //hs mjpeg
		{
			if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex -1] == 0x0a)			//160x120
			{
				Sensor_HsQcif_AltSet();
			}
			if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex -1] == 0x0b)			//176x144
			{
				Sensor_HsQcif_AltSet();
			}
			else if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex -1] == 0x14)		//320x240
			{
			 	Sensor_HsOtherWin_AltSet();
			}
			else if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex -1] == 0x16)		//352x288
			{
			 	Sensor_HsOtherWin_AltSet();
			}
			//else if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex -1] == 0x28)		//640x480
			else if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex -1] >= 0x16)	// >= 640x480
			{
				Sensor_HsOtherWin_AltSet();
				get_Jpeg_CurJpegSize(0);
			}
		}


		// 配置Video In端口的Packet Size
		bIdxTemp = reg_usb_Index;
		reg_usb_Index = VIDEO_EP_INDEX;
		// MGC_Write16(MUSB_IDX_TXMAXP, (USB20_INMAX_H_VIDEO_VAL << 8)|USB20_INMAX_L_VIDEO_VAL) ;		
		{
		unsigned int PacketSize;
		PacketSize = (USB20_INMAX_H_VIDEO_VAL << 8)|USB20_INMAX_L_VIDEO_VAL;
#if _VIDEO_EP_ == _VIDEO_EP_5_
		PacketSize |= (2 - 1) << 11;		// 0124
#elif _VIDEO_EP_ == _VIDEO_EP_4_
		PacketSize |= (1 - 1) << 11;		// 512
#endif
		MGC_Write16(MUSB_IDX_TXMAXP, (unsigned short)PacketSize);
		}

		reg_usb_Index = bIdxTemp;

		// 配置Payload Size
		// USB20_MAX_VIDEO_PAYLOAD_SIZE=(((DWORD)USB20_VIDEO_DEPTH_L_VAL)<<24)+(((DWORD)USB20_VIDEO_DEPTH_H_VAL)<<16) ;
		USB20_MAX_VIDEO_PAYLOAD_SIZE=(((DWORD)USB20_VIDEO_DEPTH_L_VAL))+(((DWORD)USB20_VIDEO_DEPTH_H_VAL)<<8) ;
		gsVideoProbeCtrl.dwMaxPayloadTransferSize = USB20_MAX_VIDEO_PAYLOAD_SIZE;	//for MAC
		
		if(bFlg_mjpeg)
		{
			gsVideoProbeCtrl.dwMaxVideoFrameSize = mjpeg_winFrame_BufferSize[gsVideoProbeCtrl.bFrameIndex-1];
		}
		else
		{
			gsVideoProbeCtrl.dwMaxVideoFrameSize = yuv_winFrame_BufferSize[gsVideoProbeCtrl.bFrameIndex-1];
		}
	}
	
	//gsStillProbeCtrl.bFormatIndex = gsVideoProbeCtrl.bFormatIndex;	//此语句会导致双格式MJPEG不拍照
	//gsStillProbeCtrl.bFrameIndex = 	gsVideoProbeCtrl.bFrameIndex;	//此语句会导致双格式MJPEG不拍照
	//gsStillProbeCtrl.dwMaxPayloadTransferSize = gsVideoProbeCtrl.dwMaxPayloadTransferSize;
	//gsStillProbeCtrl.dwMaxVideoFrameSize = gsVideoProbeCtrl.dwMaxVideoFrameSize;

#if MASK_VER_2
	//解决双格式时YUV向MJPEG格式切换时的	BUG，要点如下：
	//1、利用gsVideoProbeCtrl.bFormatIndex判断即将要求的格式；
	//2、根据所要求的格式配置gsVideoProbeCtrl.dwMaxVideoFrameSize
	if(gsDeviceStatus.bUsb_HighSpeed == 1)		//hs
	{
		if(gsDeviceStatus.bDualFormatSel)	//单格式, yuv or mjpeg
		{
			if(gsDeviceStatus.bFormatSel)	//yuv
				gsVideoProbeCtrl.dwMaxVideoFrameSize = yuv_winFrame_BufferSize[gsVideoProbeCtrl.bFrameIndex-1];
			else		//mjpeg
				gsVideoProbeCtrl.dwMaxVideoFrameSize = mjpeg_winFrame_BufferSize[gsVideoProbeCtrl.bFrameIndex-1];
		}
		else	//双格式, yuv+mjpeg	
		{
			if(bDualFormatOrder)
			{
				if(gsVideoProbeCtrl.bFormatIndex == 1)	//hs yuv
					gsVideoProbeCtrl.dwMaxVideoFrameSize = yuv_winFrame_BufferSize[gsVideoProbeCtrl.bFrameIndex-1];
				else if(gsVideoProbeCtrl.bFormatIndex == 2)	//hs mjpeg
					gsVideoProbeCtrl.dwMaxVideoFrameSize = mjpeg_winFrame_BufferSize[gsVideoProbeCtrl.bFrameIndex-1];
			}
			else
			{
				if(gsVideoProbeCtrl.bFormatIndex == 1)	//hs yuv
					gsVideoProbeCtrl.dwMaxVideoFrameSize = mjpeg_winFrame_BufferSize[gsVideoProbeCtrl.bFrameIndex-1];
				else if(gsVideoProbeCtrl.bFormatIndex == 2)	//hs mjpeg
					gsVideoProbeCtrl.dwMaxVideoFrameSize = yuv_winFrame_BufferSize[gsVideoProbeCtrl.bFrameIndex-1];
			}
		}
	}
#endif
}




//需要外挂更改默认值为VGA窗口下的处理
void Sensor_Default_Set_DspWindow(void)
{
	BYTE idata jpegWinIdx;

	if(gsDeviceStatus.bUsb_HighSpeed)	//hs
	{
		if(bFlg_mjpeg == 1)		//hs mjpeg
		{
			if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x50)	//1920x1080
			{
				jpegWinIdx = 14;
			}
			else if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x40)	//1280x1024
			{
		
				jpegWinIdx = 13;
			}
			else if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x3C)	//1280x960
			{
			
				jpegWinIdx = 12;
			}
			else if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x32)	//1280x800
			{
		
				jpegWinIdx = 11;
			}
			else if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x2D)	//1280x720
			{
		
				jpegWinIdx = 10;
			}
			else if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x30)	//1024x768
			{	
				jpegWinIdx = 9;
			}
			else if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x25)	//800x600
			{
	
				jpegWinIdx = 8;
			}
			else if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x20)	//720x576
			{
				jpegWinIdx = 7;
			}
			else if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x28)	//640x480
			{
		
				jpegWinIdx = 6;
			}
			else if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x90)	//640x400
			{
	
				jpegWinIdx = 5;
			}
			else if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x68)	//640x360
			{
		
				jpegWinIdx = 4;
			}
			else if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x16)	//352x288
			{
		
				jpegWinIdx = 3;
			}
			else if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x14)	//320x240
			{
				jpegWinIdx = 2;
			}
			else if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x0b)	//176x144
			{
		
				jpegWinIdx = 1;
			}
			else if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x0a)	//160x120
			{
		
				jpegWinIdx = 0;
			}
			else	//640x480
			{
		
				jpegWinIdx = 6;
			}
		}
		else if(bFlg_mjpeg == 0)	//hs yuv
		{
#ifdef YUV_SUPPORT
			if(yuv_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x40)	//1280x1024
			{
		
				jpegWinIdx = 13;
			}
			else if(yuv_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x3C)	//1280x960
			{
		
				jpegWinIdx = 12;
			}
			else if(yuv_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x32)	//1280x800
			{
		
				jpegWinIdx = 11;
			}
			else if(yuv_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x2D)	//1280x720
			{
		
				jpegWinIdx = 10;
			}
			else if(yuv_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x30)	//1024x768
			{
		
				jpegWinIdx = 9;
			}
			else if(yuv_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x25)	//800x600
			{
		
				jpegWinIdx = 8;
			}
			else if(yuv_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x20)	//720x576
			{
		
				jpegWinIdx = 7;
			}
			else if(yuv_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x28)	//640x480
			{
		
				jpegWinIdx = 6;
			}
			else if(yuv_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x90)	//640x400
			{
		
				jpegWinIdx = 5;
			}
			else if(yuv_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x68)	//640x360
			{
		
				jpegWinIdx = 4;
			}
			else if(yuv_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x16)	//352x288
			{
		
				jpegWinIdx = 3;
			}
			else if(yuv_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x14)	//320x240
			{
		
				jpegWinIdx = 2;
			}
			else if(yuv_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x0b)	//176x144
			{
		
				jpegWinIdx = 1;
			}
			else if(yuv_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x0a)	//160x120
			{
		
				jpegWinIdx = 0;
			}
			else	//640x480
			{
		
				jpegWinIdx = 6;
			}	
#endif
		}
	}
	else	//fs mjpeg
	{
		if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x28)	//640x480
		{
	
			jpegWinIdx = 6;
		}
		else if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x16)	//352x288
		{
	
			jpegWinIdx = 3;
		}
		else if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x14)	//320x240
		{
	
			jpegWinIdx = 2;
		}
		else if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x0b)	//176x144
		{
	
			jpegWinIdx = 1;
		}
		else if(mjpeg_winFrame_order[gsVideoProbeCtrl.bFrameIndex - 1] == 0x0a)	//160x120
		{
	
			jpegWinIdx = 0;
		}
		else	//640x480
		{
	
			jpegWinIdx = 6;
		}
	}

	if(bFlg_mjpeg)
	{
		jpeg_reg_1_2_3_set(jpegWinIdx);
		//jpeg_reg_0_set_start(1);	
	}
	else
	{
		//jpeg_module_enable(0);
		//jpeg_reg_0_set_start(0);
	}
}







#if MASK_VER_2



// UVC接口启动
void setInterfaceStart(void)
{
	// 1、置video启动标记
	gsVideoStatus.bStartVideo = TRUE;

	RamBufferCtrlForOpenWindow();
}

// UVC接口停止
void setInterfaceStop(void)
{
	if(gsVideoStatus.bStartVideo == FALSE)
		return;
	
	// 1、置video停止标记
	gsVideoStatus.bStartVideo = FALSE;

	//5、usb ep5 ctrl
	reg_usb_Index = VIDEO_EP_INDEX;
	
	// reg_UnderRun =0;
	// reg_InFlushFifo = 1;
	//MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_FlushFIFO) ;
	//MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_FlushFIFO) ;
	//MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)&~TXCSRL_FIFONotEmpty) ;
	MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)&~TXCSRL_UnderRun) ;

#if _PAYLOAD_PIPE_ == _PAYLOAD_PIPE_BULK_
	MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_ClrDataTog) ;
#endif
}
#endif
