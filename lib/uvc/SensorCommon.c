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
	0x20,	// 320X240/176X144/160X120������USB1.0�µĴ������� - USB11_INMAX_L_VIDEO_VAL
	0x02,	// 320X240/176X144/160X120������USB1.0�µĴ������� - USB11_INMAX_H_VIDEO_VAL
	0x20,	// 320X240/176X144/160X120������USB1.0�µĴ������� - USB11_VIDEO_DEPTH_L_VAL
	0x02,	// 320X240/176X144/160X120������USB1.0�µĴ������� - USB11_VIDEO_DEPTH_H_VAL

	0x20,	// 352X288������USB1.0�µĴ������� - USB11_INMAX_L_VIDEO_VAL
	0x02,	// 352X288������USB1.0�µĴ������� - USB11_INMAX_H_VIDEO_VAL
	0x20,	// 352X288������USB1.0�µĴ������� - USB11_VIDEO_DEPTH_L_VAL
	0x02,	// 352X288������USB1.0�µĴ������� - USB11_VIDEO_DEPTH_H_VAL

	0x38,	// ��������(>352x288)��USB1.0�µĴ������� - USB11_INMAX_L_VIDEO_VAL
	0x03,	// ��������(>352x288)��USB1.0�µĴ������� - USB11_INMAX_H_VIDEO_VAL
	0x38,	// ��������(>352x288)��USB1.0�µĴ������� - USB11_VIDEO_DEPTH_L_VAL
	0x03,	// ��������(>352x288)��USB1.0�µĴ������� - USB11_VIDEO_DEPTH_H_VAL
};

BYTE code * xdata pFs_AltSet_addr;

BYTE code Hs_AltSet[8] =
{
	0x80,	// 176x144/160x120������USB2.0�µĴ������� - USB20_INMAX_L_VIDEO_VAL
	0x0a,	// 176x144/160x120������USB2.0�µĴ������� - USB20_INMAX_H_VIDEO_VAL
	0x00,	// 176x144/160x120������USB2.0�µĴ������� - USB20_VIDEO_DEPTH_L_VAL
	0x05,	// 176x144/160x120������USB2.0�µĴ������� - USB20_VIDEO_DEPTH_H_VAL

	// USB 2.0����ģʽ�� Max Packet Size �� Payload Size����

#if _PAYLOAD_PIPE_ == _PAYLOAD_PIPE_ISO_	
	// Packet Size����
	0x00,	// ����������USB2.0�µĴ������� - USB11_INMAX_L_VIDEO_VAL
	0x04,	// ����������USB2.0�µĴ������� - USB11_INMAX_H_VIDEO_VAL
	
	// Payload Size����
	0xb8,	// ����������USB2.0�µĴ������� - USB11_VIDEO_DEPTH_L_VAL
//	0x0b,	// ����������USB2.0�µĴ������� - USB11_VIDEO_DEPTH_H_VAL
	0x03,	// ����������USB2.0�µĴ������� - USB11_VIDEO_DEPTH_H_VAL
#elif _PAYLOAD_PIPE_ == _PAYLOAD_PIPE_BULK_
	// Packet Size����, 512 for Bulk In Ep
	0x00,	// ����������USB2.0�µĴ������� - USB11_INMAX_L_VIDEO_VAL
	0x02,	// ����������USB2.0�µĴ������� - USB11_INMAX_H_VIDEO_VAL

#if _VIDEO_EP_ == _VIDEO_EP_4_
	// Payload Size����,512
	0x00,	// ����������USB2.0�µĴ������� - USB11_VIDEO_DEPTH_L_VAL
	0x02,	// ����������USB2.0�µĴ������� - USB11_VIDEO_DEPTH_H_VAL
#elif _VIDEO_EP_ == _VIDEO_EP_5_
	// Payload Size����,1024
	0x00, // ����������USB2.0�µĴ������� - USB11_VIDEO_DEPTH_L_VAL
#ifdef UVC_PIO	
	0x04, // ����������USB2.0�µĴ������� - USB11_VIDEO_DEPTH_H_VAL
#else
					// PayloadֵԽ��, UVC֡����H264�����֡��Խ�ӽ�.
					// PayloadֵԽС, UVC֡��ԽС, H264֡��Խ��. ��PayloadС��0x800ʱ,H264��֡�ʹ̶�Ϊ30֡, UVC֡��<16֡
	//0x80,		// �������ڴ��Payload data
	//0x20		// 380MHz DDR, H264����֡��Ϊ27֡, UVC֡��(720P)Ϊ(26~28)֡����	
	//0x10		// 380MHz DDR, ������0x20һ��
	//0x0E			// 380MHz DDR, 
	//0x0c		// 380MHz DDR, H264����֡��Ϊ28.5, UVC֡��(720P)Ϊ(18~24)֡����
	0x08,		// �������ڴ��Payload data, ֵ̫С, UVC֡���޷���֤, ��ʱH264��֡�ʹ̶�Ϊ30֡
					//		380MHz DDR, H264����֡��Ϊ30֡, UVC֡��(720P)Ϊ16֡����
					// ʹ�ý�С��Payload, ��ISP��Ӱ�콫Ϊ��С
#endif
#endif
#endif	
};
BYTE code * xdata pHs_AltSet_addr;
#endif


void set_dsp_default()
{
	//ȷ������ֵΪĬ��ֵ��Ĭ��ֵ��ǰ���ѱ���ɵ�ǰֵ
	reg_brightness_w (gsVideoStatus.bBrightness);	//gsVideoDefault.bBrightness;
	reg_contrast_w ( gsVideoStatus.bContrast);	//gsVideoDefault.bContrast;
	reg_saturation_w ( gsVideoStatus.bSaturation);//gsVideoDefault.bSaturation;
	reg_hue_w  (gsVideoStatus.wHue);	//gsVideoDefault.wHue;
}


// 320X240/176X144/160X120������USB1.0�µĴ�������
void Sensor_FsQvga_AltSet()
{
	
	//pFs_AltSet_addr = Fs_AltSet;	//��Sensor_Common_Sel_AltSet()ִ��ǰָ����ַ

	USB11_INMAX_L_VIDEO_VAL = *(pFs_AltSet_addr+0);
	USB11_INMAX_H_VIDEO_VAL = *(pFs_AltSet_addr+1);
	USB11_VIDEO_DEPTH_L_VAL = *(pFs_AltSet_addr+2);
	USB11_VIDEO_DEPTH_H_VAL = *(pFs_AltSet_addr+3);
}

// 352X288������USB1.0�µĴ�������
void Sensor_FsCif_AltSet()
{
	
	//pFs_AltSet_addr = Fs_AltSet;	//��Sensor_Common_Sel_AltSet()ִ��ǰָ����ַ

	USB11_INMAX_L_VIDEO_VAL = *(pFs_AltSet_addr+4);
	USB11_INMAX_H_VIDEO_VAL = *(pFs_AltSet_addr+5);
	USB11_VIDEO_DEPTH_L_VAL = *(pFs_AltSet_addr+6);
	USB11_VIDEO_DEPTH_H_VAL = *(pFs_AltSet_addr+7);
}

// ��������(>352x288)��USB1.0�µĴ���������JPEGѹ����ͼ���������ޣ�������֧��2���豸ͬʱ����
void Sensor_FsOtherWin_AltSet()
{
	
	//pFs_AltSet_addr = Fs_AltSet;	//��Sensor_Common_Sel_AltSet()ִ��ǰָ����ַ

	USB11_INMAX_L_VIDEO_VAL = *(pFs_AltSet_addr+8);
	USB11_INMAX_H_VIDEO_VAL = *(pFs_AltSet_addr+9);
	USB11_VIDEO_DEPTH_L_VAL = *(pFs_AltSet_addr+10);
	USB11_VIDEO_DEPTH_H_VAL = *(pFs_AltSet_addr+11);
}

// 176x144/160x120������USB2.0�µĴ�������
void Sensor_HsQcif_AltSet()
{
	
	//pHs_AltSet_addr = Hs_AltSet;	//��Sensor_Common_Sel_AltSet()ִ��ǰָ����ַ

	USB20_INMAX_L_VIDEO_VAL = *(pHs_AltSet_addr+0);
	USB20_INMAX_H_VIDEO_VAL = *(pHs_AltSet_addr+1);
	USB20_VIDEO_DEPTH_L_VAL = *(pHs_AltSet_addr+2);
	USB20_VIDEO_DEPTH_H_VAL = *(pHs_AltSet_addr+3);
}

// ����������USB2.0�µĴ�������
void Sensor_HsOtherWin_AltSet()
{
	
	//pHs_AltSet_addr = Hs_AltSet;	//��Sensor_Common_Sel_AltSet()ִ��ǰָ����ַ

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

//��ĳЩ������Ҫ���Ĵ������ã���˺�������Ҫ�����
//���ݲ�ͬ���ڸ������¼�������ֵ
// USB11_INMAX_L_VIDEO_VAL & USB11_INMAX_H_VIDEO_VAL - Ӱ��reg_usb_InMaxPL & reg_usb_InMaxPH �Լ�USB11_MAX_VIDEO_PAYLOAD_SIZE
// USB11_VIDEO_DEPTH_L_VAL & USB11_VIDEO_DEPTH_H_VAL - Ӱ��video_depth_1frameL & video_depth_1frameH
// USB20_INMAX_L_VIDEO_VAL & USB20_INMAX_H_VIDEO_VAL - Ӱ��reg_usb_InMaxPL & reg_usb_InMaxPH �Լ�USB20_MAX_VIDEO_PAYLOAD_SIZE
// USB20_VIDEO_DEPTH_L_VAL & USB20_VIDEO_DEPTH_H_VAL - Ӱ��video_depth_1frameL & video_depth_1frameH
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


		// ����Video In�˿ڵ�Packet Size
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

		// ����Payload Size
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
	
	//gsStillProbeCtrl.bFormatIndex = gsVideoProbeCtrl.bFormatIndex;	//�����ᵼ��˫��ʽMJPEG������
	//gsStillProbeCtrl.bFrameIndex = 	gsVideoProbeCtrl.bFrameIndex;	//�����ᵼ��˫��ʽMJPEG������
	//gsStillProbeCtrl.dwMaxPayloadTransferSize = gsVideoProbeCtrl.dwMaxPayloadTransferSize;
	//gsStillProbeCtrl.dwMaxVideoFrameSize = gsVideoProbeCtrl.dwMaxVideoFrameSize;

#if MASK_VER_2
	//���˫��ʽʱYUV��MJPEG��ʽ�л�ʱ��	BUG��Ҫ�����£�
	//1������gsVideoProbeCtrl.bFormatIndex�жϼ���Ҫ��ĸ�ʽ��
	//2��������Ҫ��ĸ�ʽ����gsVideoProbeCtrl.dwMaxVideoFrameSize
	if(gsDeviceStatus.bUsb_HighSpeed == 1)		//hs
	{
		if(gsDeviceStatus.bDualFormatSel)	//����ʽ, yuv or mjpeg
		{
			if(gsDeviceStatus.bFormatSel)	//yuv
				gsVideoProbeCtrl.dwMaxVideoFrameSize = yuv_winFrame_BufferSize[gsVideoProbeCtrl.bFrameIndex-1];
			else		//mjpeg
				gsVideoProbeCtrl.dwMaxVideoFrameSize = mjpeg_winFrame_BufferSize[gsVideoProbeCtrl.bFrameIndex-1];
		}
		else	//˫��ʽ, yuv+mjpeg	
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




//��Ҫ��Ҹ���Ĭ��ֵΪVGA�����µĴ���
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



// UVC�ӿ�����
void setInterfaceStart(void)
{
	// 1����video�������
	gsVideoStatus.bStartVideo = TRUE;

	RamBufferCtrlForOpenWindow();
}

// UVC�ӿ�ֹͣ
void setInterfaceStop(void)
{
	if(gsVideoStatus.bStartVideo == FALSE)
		return;
	
	// 1����videoֹͣ���
	gsVideoStatus.bStartVideo = FALSE;

	//5��usb ep5 ctrl
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
