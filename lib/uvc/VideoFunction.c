#include <xm_proj_define.h>

#include "common.h"
#include "descript.h"
#include "requests.h"
#include "descript_def.h"
#include "sensorlib.h"
#include <stdio.h>

#include "CdrRequest.h"
#include "xm_uvc_socket.h"

#define POWER			0x03
#define OUT_OF_RANGE		0x04
#define INVALID_UNIT		0x05
#define INVALID_CONTROL	0x06
#define INVALID_REQUEST	0x07

BYTE xdata gbVideoError;
BYTE xdata gbVideoErrorLast;


BYTE xdata bFlg_needFormatChange = 0; 	// 0 - 不需要模式切换； 1 - YUV切至MJPEG；  2 - MJPEG切至YUV

void ERROR_VIDEO_OUT_RANGE()
{
	gbVideoError = OUT_OF_RANGE;
	//gbError = TRUE;	
}

void ERROR_VIDEO_INVALID_UNIT()
{
	gbVideoError = INVALID_UNIT;
	gbError = TRUE;
}

void ERROR_VIDEO_INVALID_CONTROL()
{
	gbVideoError = INVALID_CONTROL;
	gbError = TRUE;
}

//void ERROR_VIDEO_INVALID_REQUEST()
//{
//	gbVideoError = INVALID_REQUEST;
//	gbError = TRUE;
//}

void video_control_set_cur()
{
	WORD wTemp;

	//printf ("video_control_set_cur %x %x\n", MSB(gsUsbRequest.wIndex), MSB(gsUsbRequest.wValue));
	switch (MSB(gsUsbRequest.wIndex))
	{
		case VC_INTERFACE_CONTROL :
			switch (MSB(gsUsbRequest.wValue))
			{
				case VC_VIDEO_POWER_MODE_CONTROL :	 //need debug
					usb_fetch_out(1);
					break;
				case VC_REQUEST_ERROR_CODE_CONTROL :
					usb_fetch_out(1);
					if (gbBuffer[0] <= 0x07)
						gbVideoError = gbBuffer[0];
					else
						ERROR_VIDEO_OUT_RANGE();
					break;
				default :
					ERROR_VIDEO_INVALID_CONTROL();
					break;
			}			
			break;
			
		case VC_CAMERA_TERMINAL :
			switch (MSB(gsUsbRequest.wValue))
			{
				case CT_AE_MODE_CONTROL :	//D0:Manual  D1:Auto  D2:ShutterPriority  D3:AperturePriority
					usb_fetch_out(1);
					// printf ("SET CT_AE_MODE_CONTROL %x\n", gbBuffer[0]);
					if(gbBuffer[0] == 0x02 || gbBuffer[0] == 0x04)	//Auto or ShutterPriority Mode
					{
						gsVideoStatus.bAEMode = gbBuffer[0];
						gsVideoStatus.bmAdjust |= Adj_AEMode;
						bmAttrAdjust |= Adj_AEMode;
					}
					else
					{
						ERROR_VIDEO_OUT_RANGE();
					}
					break;
				case CT_EXPOSURE_TIME_ABSOLUTE_CONTROL :	//Unit : 100us
					usb_fetch_out(4);
					wTemp = (WORD)gbBuffer[0] | (WORD)(gbBuffer[1] << 8);
					if ((gbBuffer[3] == 0x00) && (gbBuffer[2] == 0x00) && (wTemp <= gsVideoLimit.wMaxExposureTime))
					{
						gsVideoStatus.wExposureTime = wTemp;
						gsVideoStatus.bmAdjust |= Adj_ExposureTime;
						bmAttrAdjust |= Adj_ExposureTime;
					}
					else
					{
						ERROR_VIDEO_OUT_RANGE();
					}	
					break;
				/*
				case CT_IRIS_ABSOLUTE_CONTROL :		//Unit : fstop * 100	//???
					usb_fetch_out(2);
					ERROR_VIDEO_OUT_RANGE();
					break;
				*/
				case CT_ROLL_ABSOLUTE_CONTROL :		//Unit : 1 degree,  signed number, {-180 ~ 180}
					usb_fetch_out(2);
					if ((gbBuffer[1] == 0x00) && (gbBuffer[0] <= 180))
					{
						gsVideoStatus.bRoll = gbBuffer[0];
						gsVideoStatus.bmAdjust |= Adj_Roll;
						bmAttrAdjust |= Adj_Roll;
					}
					else
					{
						ERROR_VIDEO_OUT_RANGE();
					}					
					break;
				default :
					ERROR_VIDEO_INVALID_CONTROL();					
					break;
			}			
			break;
			
		case VC_PROCESSING_UNIT :		//set cur
			switch (MSB(gsUsbRequest.wValue))
			{
				case PU_BACKLIGHT_COMPENSATION_CONTROL :
					usb_fetch_out(2);
					if((gbBuffer[1] == 0) &&
							(gbBuffer[0] >= gsVideoLimit.bMinBacklightComp) &&
							(gbBuffer[0] <= gsVideoLimit.bMaxBacklightComp))
					{
						gsVideoStatus.bBacklightComp = gbBuffer[0];
						gsVideoStatus.bmAdjust |= Adj_BacklightComp;
						bmAttrAdjust |= Adj_BacklightComp;
					}
					else
					{
						ERROR_VIDEO_OUT_RANGE();
					}

					break;
				case PU_BRIGHTNESS_CONTROL :	//set cur //signed number
					usb_fetch_out(2);
					// printf ("SET PU_BRIGHTNESS_CONTROL %x %x\n", gbBuffer[0], gbBuffer[1]);
#if 1//_XM_PROJ_ == _XM_PROJ_DVR_4_SENSOR_D1_
					// CDR 私有协议处理
					XMSYS_CdrPrivateProtocolHandler (gbBuffer[0], gbBuffer[1]);
#else
					if (gbBuffer[1] == 0)
					{
						gsVideoStatus.bBrightness = gbBuffer[0];
						gsVideoStatus.bmAdjust |= Adj_Brightness;
						bmAttrAdjust |= Adj_Brightness;
					}
					else
					{
						ERROR_VIDEO_OUT_RANGE();
					}
#endif
					break;
				case PU_CONTRAST_CONTROL :
					usb_fetch_out(2);
					if (gbBuffer[1] == 0)
					{
						gsVideoStatus.bContrast = gbBuffer[0];
						gsVideoStatus.bmAdjust |= Adj_Contrast;
						bmAttrAdjust |= Adj_Contrast;
					}
					else
					{
						ERROR_VIDEO_OUT_RANGE();
					}					
					break;

				case PU_GAIN_CONTROL :
					usb_fetch_out(2);		  		//将数据接收至gbBuffer中
					break;

				case PU_POWER_LINE_FREQUENCY_CONTROL :
					usb_fetch_out(1);
					//if (gbBuffer[0] <= gsVideoLimit.bMaxPowerLineFreq)
					if (gbBuffer[0] <= 0x03)
					{
						//gsVideoStatus.bPowerLineFreq = gbBuffer[0];
						//gsVideoStatus.bFlickerFlag = gbBuffer[0];
						//gsVideoStatus.bFlickerFlag = 1;
						if((bDebugFlg & 0x08) == 0)
						{
							gsDeviceStatus.bDeflickMode = gbBuffer[0];
							gsVideoStatus.bmAdjust |= Adj_PowerLineFreq;
							bmAttrAdjust |= Adj_PowerLineFreq;
						}
					}
					else
					{
						ERROR_VIDEO_OUT_RANGE();
					}					
					gbBuffer[0] = 0;
					break;
				case PU_HUE_CONTROL :	//set cur //Unit : 1/100 degree, signed number, {-1800~1800}
					usb_fetch_out(2);
					if (((gbBuffer[1] == 0x00) && (gbBuffer[0] < 0x80))
					    || ((gbBuffer[1] == 0xFF) && (gbBuffer[0] > 0x80)))
					{
						gsVideoStatus.wHue = (WORD)gbBuffer[0] | (WORD)(gbBuffer[1] << 8);
						gsVideoStatus.bmAdjust |= Adj_Hue;
						bmAttrAdjust |= Adj_Hue;
					}
					else
					{
						ERROR_VIDEO_OUT_RANGE();
					}					
					break;
				case PU_SATURATION_CONTROL :
					usb_fetch_out(2);
					if (gbBuffer[1] == 0)
					{
						gsVideoStatus.bSaturation = gbBuffer[0];
						gsVideoStatus.bmAdjust |= Adj_Saturation;
						bmAttrAdjust |= Adj_Saturation;
					}
					else
					{
						ERROR_VIDEO_OUT_RANGE();
					}					
					break;
				case PU_SHARPNESS_CONTROL :
					usb_fetch_out(2);
					if ((gbBuffer[1] == 0) && (gbBuffer[0] <= gsVideoLimit.bMaxSharpness))
					{
						gsVideoStatus.bSharpness = gbBuffer[0];
						gsVideoStatus.bmAdjust |= Adj_Sharpness;
						bmAttrAdjust |= Adj_Sharpness;
					}
					else
					{
						ERROR_VIDEO_OUT_RANGE();
					}			
					break;
				case PU_GAMMA_CONTROL :		//Unit : 1/100,  {1~500}
					usb_fetch_out(2);
					if ((gbBuffer[1] == 0) && (gbBuffer[0] >= gsVideoLimit.bMinGamma) && (gbBuffer[0] <= gsVideoLimit.bMaxGamma))
					{
						gsVideoStatus.bGamma = gbBuffer[0];
						gsVideoStatus.bmAdjust |= Adj_Gamma;
						bmAttrAdjust |= Adj_Gamma;
					}
					else
					{
						ERROR_VIDEO_OUT_RANGE();
					}			
					break;
				case PU_WHITE_BALANCE_TEMPERATURE_CONTROL:
					usb_fetch_out(2);
					gsVideoStatus.wWBC = ((WORD)(gbBuffer[1])<<8) + gbBuffer[0];
					
					if(gsVideoStatus.bWBAuto == 0)
					{
						gsVideoStatus.bmAdjust |= Adj_WBC;
						bmAttrAdjust |= Adj_WBC;
					}
					//else
					//{
					//	ERROR_VIDEO_OUT_RANGE();
					//}			
					break;
				case PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL:
					usb_fetch_out(1);
					gsVideoStatus.bWBAuto = gbBuffer[0];
					break;
				/*
				case PU_WHITE_BALANCE_COMPONENT_CONTROL:
					
					break;
				case PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL:
					
					break;
				*/
				default :
					ERROR_VIDEO_INVALID_CONTROL();					
					break;
			}
			break;
		/*
		case VC_SELECTOR_UNIT :
			usb_fetch_out(1);
			if(gbBuffer[0] == 0x01)
				//select unit 1
			else if(gbBuffer[1] == 0x02)
				//select unit 2	
			else
				ERROR_VIDEO_INVALID_CONTROL();
			break;
		*/	
		default :
			ERROR_VIDEO_INVALID_UNIT();
			break;
	}
}

void video_control_get_cur()
{
	switch (MSB(gsUsbRequest.wIndex))
	{
		case VC_INTERFACE_CONTROL :
			switch (MSB(gsUsbRequest.wValue))
			{
				case VC_VIDEO_POWER_MODE_CONTROL :
					gbBuffer[0] = 0x00;
					usb_setup_in(1);
					break;
				case VC_REQUEST_ERROR_CODE_CONTROL :
					gbBuffer[0] = gbVideoErrorLast;
					usb_setup_in(1);					
					break;
				default :
					ERROR_VIDEO_INVALID_CONTROL();
					break;
			}			
			break;
			
		case VC_CAMERA_TERMINAL :
			printf ("video_control_get_cur VC_CAMERA_TERMINAL %x\n", MSB(gsUsbRequest.wValue));
			switch (MSB(gsUsbRequest.wValue))
			{
				case CT_AE_MODE_CONTROL :
					gbBuffer[0] = gsVideoStatus.bAEMode;
					printf ("GET CT_AE_MODE_CONTROL %x\n", gbBuffer[0]);
					usb_setup_in(1);
					break;
				case CT_EXPOSURE_TIME_ABSOLUTE_CONTROL :
					gbBuffer[0] = (BYTE)gsVideoStatus.wExposureTime;
					gbBuffer[1] = (BYTE)(gsVideoStatus.wExposureTime >> 8);
					gbBuffer[2] = 0x00;
					gbBuffer[3] = 0x00;
					usb_setup_in(4);
					break;
				/*
				case CT_IRIS_ABSOLUTE_CONTROL :
					gbBuffer[0] = 0xFF;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				*/
				case CT_ROLL_ABSOLUTE_CONTROL :
					gbBuffer[0] = gsVideoStatus.bRoll;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);						
					break;
				default :
					ERROR_VIDEO_INVALID_CONTROL();
					break;
			}			
			break;
		case VC_PROCESSING_UNIT :	//get cur
			switch (MSB(gsUsbRequest.wValue))
			{
				case PU_BACKLIGHT_COMPENSATION_CONTROL :
					gbBuffer[0] = gsVideoStatus.bBacklightComp;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				case PU_BRIGHTNESS_CONTROL :  	//get cur
#if 1
					{
					unsigned short resp_frame = XMSYS_CdrGetResponseFrame ();
					gbBuffer[0] = (unsigned char)(resp_frame);
					gbBuffer[1] = (unsigned char)(resp_frame >> 8);
					//printf ("RESP %02x %02x\n", gbBuffer[0], gbBuffer[1]);
					}
#else
					gsVideoStatus.bBrightness = reg_brightness_r();
					gbBuffer[0] = gsVideoStatus.bBrightness;
					gbBuffer[1] = 0x00;
#endif
					usb_setup_in(2);				
					break;
				case PU_CONTRAST_CONTROL :
					gsVideoStatus.bContrast = reg_contrast_r();
					gbBuffer[0] = gsVideoStatus.bContrast;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);				
					break;
				case PU_GAIN_CONTROL :
					gbBuffer[0] = gsVideoStatus.wGain & 0xFF;
					gbBuffer[1] = gsVideoStatus.wGain >> 8;
					usb_setup_in(2);				
					break;
				case PU_POWER_LINE_FREQUENCY_CONTROL :
					gbBuffer[0] = gsVideoStatus.bPowerLineFreq;
					usb_setup_in(1);				
					break;
				case PU_HUE_CONTROL : 	//get cur
					gsVideoStatus.wHue = (WORD)reg_hue_r();
					
					if((reg_hue_r() & 0x80) == 0x80)	
					{
						gbBuffer[0] = 0xFF - (BYTE)gsVideoStatus.wHue + 0x81;
						gbBuffer[1] = 0xff;
					}
					else
					{
						gbBuffer[0] = (BYTE)gsVideoStatus.wHue;
						gbBuffer[1] = 0x00;
					}
					usb_setup_in(2);				
					break;
				case PU_SATURATION_CONTROL :
					gsVideoStatus.bSaturation = reg_saturation_r();
					gbBuffer[0] = gsVideoStatus.bSaturation;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);				
					break;					
				case PU_SHARPNESS_CONTROL :
					//gsVideoStatus.bSharpness = reg_peaking_coefl ;
					gbBuffer[0] = gsVideoStatus.bSharpness;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);			
					break;
				case PU_GAMMA_CONTROL :
					gbBuffer[0] = gsVideoStatus.bGamma;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);			
					break;
				case PU_WHITE_BALANCE_TEMPERATURE_CONTROL:
					gbBuffer[0] = gsVideoStatus.wWBC;
					gbBuffer[1] = (gsVideoStatus.wWBC)>>8;
					usb_setup_in(2);
					break;
				case PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL:
					gbBuffer[0] = gsVideoStatus.bWBAuto;					
					usb_setup_in(1);
					break;
				default :
					ERROR_VIDEO_INVALID_CONTROL();
					break;
			}
			break;
			
		default :
			ERROR_VIDEO_INVALID_UNIT();			
			break;
	}
}

void video_control_get_min()
{
	switch (MSB(gsUsbRequest.wIndex))
	{
		case VC_INTERFACE_CONTROL :
			switch (MSB(gsUsbRequest.wValue))
			{
				case VC_VIDEO_POWER_MODE_CONTROL :
					gbBuffer[0] = 0x00;
					usb_setup_in(1);
					break;
				case VC_REQUEST_ERROR_CODE_CONTROL :
					gbBuffer[0] = 0x00;
					usb_setup_in(1);
					break;
				default :
					ERROR_VIDEO_INVALID_CONTROL();
					break;
			}			
			break;
			
		case VC_CAMERA_TERMINAL :
			switch (MSB(gsUsbRequest.wValue))
			{
				case CT_AE_MODE_CONTROL :
					gbBuffer[0] = 0x02;
					usb_setup_in(1);
					break;
				case CT_EXPOSURE_TIME_ABSOLUTE_CONTROL :
					gbBuffer[0] = 0x01;
					gbBuffer[1] = 0x00;
					gbBuffer[2] = 0x00;
					gbBuffer[3] = 0x00;
					usb_setup_in(4);
					break;
				/*
				case CT_IRIS_ABSOLUTE_CONTROL :
					gbBuffer[0] = 0xFF;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				*/
				case CT_ROLL_ABSOLUTE_CONTROL :
					gbBuffer[0] = 0x00;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				default :
					ERROR_VIDEO_INVALID_CONTROL();					
					break;
			}			
			break;
		case VC_PROCESSING_UNIT :
			switch (MSB(gsUsbRequest.wValue))
			{
				case PU_BACKLIGHT_COMPENSATION_CONTROL :
					gbBuffer[0] = gsVideoLimit.bMinBacklightComp;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				case PU_BRIGHTNESS_CONTROL :	//get min
					gbBuffer[0] = 0x00;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				case PU_CONTRAST_CONTROL :
					gbBuffer[0] = 0x00;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				case PU_GAIN_CONTROL :
					gbBuffer[0] = 0x00;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				case PU_POWER_LINE_FREQUENCY_CONTROL :
					gbBuffer[0] = 0x00;
					usb_setup_in(1);
					break;
				case PU_HUE_CONTROL :		//get min
					gbBuffer[0] = 0x81;	//0x80;
					gbBuffer[1] = 0xFF;
					usb_setup_in(2);
					break;
				case PU_SATURATION_CONTROL :
					gbBuffer[0] = 0x00;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				case PU_SHARPNESS_CONTROL :
					gbBuffer[0] = 0x00;	//gbBuffer[0] = 0x00;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				case PU_GAMMA_CONTROL :
					gbBuffer[0] = gsVideoLimit.bMinGamma;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				case PU_WHITE_BALANCE_TEMPERATURE_CONTROL:
					gbBuffer[0] = 0x60;	//2400
					gbBuffer[1] = 0x09;
					usb_setup_in(2);
					break;
				case PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL:
					gbBuffer[0] = 0;					
					usb_setup_in(1);
					break;
				/*
				case PU_WHITE_BALANCE_COMPONENT_CONTROL:
					break;
				case PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL:
					break;
				*/
				default :
					ERROR_VIDEO_INVALID_CONTROL();					
					break;
			}
			break;
			
		default :
			ERROR_VIDEO_INVALID_UNIT();
			break;
	}
}

void video_control_get_max()
{
	switch (MSB(gsUsbRequest.wIndex))
	{
		case VC_INTERFACE_CONTROL :
			switch (MSB(gsUsbRequest.wValue))
			{
				case VC_VIDEO_POWER_MODE_CONTROL :
					gbBuffer[0] = 0x00;
					usb_setup_in(1);
					break;
				case VC_REQUEST_ERROR_CODE_CONTROL :
					gbBuffer[0] = 0x07;
					usb_setup_in(1);
					break;
				default :
					ERROR_VIDEO_INVALID_CONTROL();
					break;
			}			
			break;
			
		case VC_CAMERA_TERMINAL :
			switch (MSB(gsUsbRequest.wValue))
			{
				case CT_AE_MODE_CONTROL :
					gbBuffer[0] = 0x04;
					usb_setup_in(1);
					break;
				case CT_EXPOSURE_TIME_ABSOLUTE_CONTROL :
					gbBuffer[0] = (BYTE)gsVideoLimit.wMaxExposureTime;
					gbBuffer[1] = (BYTE)(gsVideoLimit.wMaxExposureTime >> 8);
					gbBuffer[2] = 0x00;
					gbBuffer[3] = 0x00;
					usb_setup_in(4);
					break;
				/*
				case CT_IRIS_ABSOLUTE_CONTROL :
					gbBuffer[0] = 0xFF;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				*/
				case CT_ROLL_ABSOLUTE_CONTROL :
					gbBuffer[0] = 180;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				default :
					ERROR_VIDEO_INVALID_CONTROL();					
					break;
			}			
			break;
		case VC_PROCESSING_UNIT :
			switch (MSB(gsUsbRequest.wValue))
			{
				case PU_BACKLIGHT_COMPENSATION_CONTROL :
					gbBuffer[0] = gsVideoLimit.bMaxBacklightComp;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				case PU_BRIGHTNESS_CONTROL :	//get max
					gbBuffer[0] = 0xFF;
					gbBuffer[1] = 0x7F;
					usb_setup_in(2);
					break;
				case PU_CONTRAST_CONTROL :
					gbBuffer[0] = 0xFF;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				case PU_GAIN_CONTROL :
					gbBuffer[0] = 0xFF;
					//lin gbBuffer[1] = 0x00;
					gbBuffer[1] = 0xff;	//此外必须为0xff，才能与托盘程序正常握手, 因为必须考虑0xaa55
					usb_setup_in(2);
					break;
				case PU_POWER_LINE_FREQUENCY_CONTROL :
					gbBuffer[0] = gsVideoLimit.bMaxPowerLineFreq;
					usb_setup_in(1);
					break;
				case PU_HUE_CONTROL :		//get max
					gbBuffer[0] = 0x7F;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				case PU_SATURATION_CONTROL :
					gbBuffer[0] = 0xFF;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				case PU_SHARPNESS_CONTROL :
					gbBuffer[0] = gsVideoLimit.bMaxSharpness;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				case PU_GAMMA_CONTROL :
					gbBuffer[0] = gsVideoLimit.bMaxGamma;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				case PU_WHITE_BALANCE_TEMPERATURE_CONTROL:
					gbBuffer[0] = 0x64;	//6500
					gbBuffer[1] = 0x19;
					usb_setup_in(2);
					break;
				case PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL:
					gbBuffer[0] = 1;					
					usb_setup_in(1);
					break;
				/*
				case PU_WHITE_BALANCE_COMPONENT_CONTROL:
					break;
				case PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL:
					break;
				*/
				default :
					ERROR_VIDEO_INVALID_CONTROL();					
					break;
			}
			break;
			
		default :
			ERROR_VIDEO_INVALID_UNIT();
			break;
	}
}

void video_control_get_def()
{
	switch (MSB(gsUsbRequest.wIndex))
	{
		case VC_INTERFACE_CONTROL :
			switch (MSB(gsUsbRequest.wValue))
			{
				case VC_VIDEO_POWER_MODE_CONTROL :
					gbBuffer[0] = 0x00;
					usb_setup_in(1);
					break;
				case VC_REQUEST_ERROR_CODE_CONTROL :
					gbBuffer[0] = 0x00;
					usb_setup_in(1);
					break;
				default :
					ERROR_VIDEO_INVALID_CONTROL();
					break;
			}			
			break;
			
		case VC_CAMERA_TERMINAL :
			//printf ("VC_CAMERA_TERMINAL %d\n",  MSB(gsUsbRequest.wValue));
			switch (MSB(gsUsbRequest.wValue))
			{
				case CT_AE_MODE_CONTROL :
					gbBuffer[0] = gsVideoDefault.bAEMode;
					usb_setup_in(1);
					break;
				case CT_EXPOSURE_TIME_ABSOLUTE_CONTROL :
					gbBuffer[0] = (BYTE)gsVideoDefault.wExposureTime;
					gbBuffer[1] = (BYTE)(gsVideoDefault.wExposureTime >> 8);
					gbBuffer[2] = 0x00;
					gbBuffer[3] = 0x00;
					usb_setup_in(4);
					break;
				/*
				case CT_IRIS_ABSOLUTE_CONTROL :
					gbBuffer[0] = 0x00;	////gbBuffer[0] = 0xFF;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				*/
				case CT_ROLL_ABSOLUTE_CONTROL :
					gbBuffer[0] = gsVideoDefault.bRoll;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				default :
					ERROR_VIDEO_INVALID_CONTROL();					
					break;
			}			
			break;

		case VC_PROCESSING_UNIT :
			//printf ("VC_PROCESSING_UNIT %d\n",  MSB(gsUsbRequest.wValue));
			switch (MSB(gsUsbRequest.wValue))
			{
				case PU_BACKLIGHT_COMPENSATION_CONTROL :
					gbBuffer[0] = gsVideoDefault.bBacklightComp;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				case PU_BRIGHTNESS_CONTROL :	//get default
					gbBuffer[0] = gsVideoDefault.bBrightness;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				case PU_CONTRAST_CONTROL :
					gbBuffer[0] = gsVideoDefault.bContrast;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				case PU_GAIN_CONTROL :
					gbBuffer[0] = gsVideoDefault.wGain;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				case PU_POWER_LINE_FREQUENCY_CONTROL :
					gbBuffer[0] = gsVideoDefault.bPowerLineFreq;
					usb_setup_in(1);
					break;
				case PU_HUE_CONTROL :		 //get default
					gbBuffer[0] = (BYTE)gsVideoDefault.wHue;
					gbBuffer[1] = (BYTE)(gsVideoDefault.wHue >> 8);
					usb_setup_in(2);
					break;
				case PU_SATURATION_CONTROL :
					gbBuffer[0] = gsVideoDefault.bSaturation;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				case PU_SHARPNESS_CONTROL :
					gbBuffer[0] = gsVideoDefault.bSharpness;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				case PU_GAMMA_CONTROL :
					gbBuffer[0] = gsVideoDefault.bGamma;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				case PU_WHITE_BALANCE_TEMPERATURE_CONTROL:
					gbBuffer[0] = gsVideoDefault.wWBC;
					gbBuffer[1] = (gsVideoDefault.wWBC)>>8;
					usb_setup_in(2);
					break;
				case PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL:
					gbBuffer[0] = gsVideoDefault.bWBAuto;					
					usb_setup_in(1);
					break;
				default :
					ERROR_VIDEO_INVALID_CONTROL();					
					break;
			}
			break;
			
		default :
			ERROR_VIDEO_INVALID_UNIT();
			break;
	}
}

//获取相对于DEFAULT的相对值 ？？？？
void video_control_get_res()
{
	switch (MSB(gsUsbRequest.wIndex))
	{
		case VC_INTERFACE_CONTROL :
			switch (MSB(gsUsbRequest.wValue))
			{
				case VC_VIDEO_POWER_MODE_CONTROL :
					gbBuffer[0] = 0x01;
					usb_setup_in(1);
					break;
				case VC_REQUEST_ERROR_CODE_CONTROL :
					gbBuffer[0] = 0x01;
					usb_setup_in(1);
					break;
				default :
					ERROR_VIDEO_INVALID_CONTROL();
					break;
			}			
			break;
			
		case VC_CAMERA_TERMINAL :
			switch (MSB(gsUsbRequest.wValue))
			{
				case CT_AE_MODE_CONTROL :
					gbBuffer[0] = 0x02;
					usb_setup_in(1);					
					break;
				case CT_EXPOSURE_TIME_ABSOLUTE_CONTROL :
					gbBuffer[0] = 0x01;
					gbBuffer[1] = 0x00;
					gbBuffer[2] = 0x00;
					gbBuffer[3] = 0x00;
					usb_setup_in(4);			
					break;
				/*
				case CT_IRIS_ABSOLUTE_CONTROL :
					gbBuffer[0] = 0x01;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);
					break;
				*/
				case CT_ROLL_ABSOLUTE_CONTROL :
					gbBuffer[0] = bRollDegree;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);			
					break;
				default :
					ERROR_VIDEO_INVALID_CONTROL();					
					break;
			}			
			break;
		case VC_PROCESSING_UNIT :
			switch (MSB(gsUsbRequest.wValue))
			{
				case PU_BACKLIGHT_COMPENSATION_CONTROL :
					gbBuffer[0] = 0x01;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);			
					break;
				case PU_BRIGHTNESS_CONTROL :	//res
					gbBuffer[0] = 0x01;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);			
					break;
				case PU_CONTRAST_CONTROL :
					gbBuffer[0] = 0x01;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);			
					break;
				case PU_GAIN_CONTROL :
					gbBuffer[0] = 0x01;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);			
					break;
				case PU_POWER_LINE_FREQUENCY_CONTROL :
					gbBuffer[0] = 0x01;
					usb_setup_in(1);			
					break;
				case PU_HUE_CONTROL :	//res
					gbBuffer[0] = 0x01;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);			
					break;
				case PU_SATURATION_CONTROL :
					gbBuffer[0] = 0x01;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);			
					break;
				case PU_SHARPNESS_CONTROL :
					gbBuffer[0] = 0x01;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);			
					break;
				case PU_GAMMA_CONTROL :
					gbBuffer[0] = 0x01;
					gbBuffer[1] = 0x00;
					usb_setup_in(2);			
					break;
				case PU_WHITE_BALANCE_TEMPERATURE_CONTROL:
					gbBuffer[0] = AWB_Degree;
					gbBuffer[1] = (AWB_Degree>>8);
					usb_setup_in(2);
					break;
				
				case PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL:
					gbBuffer[0] = 0x01;
					usb_setup_in(1);
					break;
				default :
					ERROR_VIDEO_INVALID_CONTROL();					
					break;
			}
			break;
			
		default :
			ERROR_VIDEO_INVALID_UNIT();
			break;
	}
}

//长度都为1,  统一进行setup
void video_control_get_info()
{
	switch (MSB(gsUsbRequest.wIndex))
	{
		case VC_INTERFACE_CONTROL :
			switch (MSB(gsUsbRequest.wValue))
			{
				case VC_VIDEO_POWER_MODE_CONTROL :
					gbBuffer[0] = 0x07;
					break;
				case VC_REQUEST_ERROR_CODE_CONTROL :
					gbBuffer[0] = 0x07;
					break;
				default :
					ERROR_VIDEO_INVALID_CONTROL();
					break;
			}			
			break;
			
		case VC_CAMERA_TERMINAL :
			switch (MSB(gsUsbRequest.wValue))
			{
				case CT_AE_MODE_CONTROL :
					gbBuffer[0] = 0x07;
					break;
				case CT_EXPOSURE_TIME_ABSOLUTE_CONTROL :
					if (gsVideoStatus.bAEMode == 0x02)
						gbBuffer[0] = 0x0F;
					else
						gbBuffer[0] = 0x07;
					break;
				/*
				case CT_IRIS_ABSOLUTE_CONTROL :	  	//hally, 此外处理可加快枚举速度
					gbBuffer[0] = 0x07;
					break;
				*/
				case CT_ROLL_ABSOLUTE_CONTROL :
					gbBuffer[0] = 0x07;
					break;
				default :
					ERROR_VIDEO_INVALID_CONTROL();					
					break;
			}			
			break;
		case VC_PROCESSING_UNIT :
			switch (MSB(gsUsbRequest.wValue))
			{
				case PU_BACKLIGHT_COMPENSATION_CONTROL :
					gbBuffer[0] = 0x07;
					break;
				case PU_BRIGHTNESS_CONTROL :	//info
					gbBuffer[0] = 0x07;
					break;
				case PU_CONTRAST_CONTROL :
					gbBuffer[0] = 0x07;
					break;
				case PU_GAIN_CONTROL :
					gbBuffer[0] = 0x07;
					break;
				case PU_POWER_LINE_FREQUENCY_CONTROL :
					gbBuffer[0] = 0x07;
					break;
				case PU_HUE_CONTROL :	//info
					gbBuffer[0] = 0x07;
					break;
				case PU_SATURATION_CONTROL :
					gbBuffer[0] = 0x07;
					break;
				case PU_SHARPNESS_CONTROL :
					gbBuffer[0] = 0x07;
					break;
				case PU_GAMMA_CONTROL :
					gbBuffer[0] = 0x07;
					break;
				case PU_WHITE_BALANCE_TEMPERATURE_CONTROL:
					gbBuffer[0] = 0x03;
					break;
				case PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL:
					gbBuffer[0] = 0x07;
					break;
				default :
					ERROR_VIDEO_INVALID_CONTROL();					
					break;
			}
			break;

			
		default :
			ERROR_VIDEO_INVALID_UNIT();
			break;
	}

	// 参考USB_Video_Class_1.1.pdf 
	// 2.4.4 Control Transfer and Request Processing (P37、38、39页)
	// 4.1.2 Get Request (P75、P76)
	// 关闭"Status Interrupt Endpoint".端口，不需要"asynchronous control" "Autoupdate control"
	// 即所有控制均为同步操作，设备没有自动控制功能
	// 通过 GEI_INFO 协议，D2 1=Disabled due to automatic mode (under device control)
	gbBuffer[0] &= ~0x04;


	if (!gbVideoError)
		usb_setup_in(1);
}

//长度都为2,  统一进行setup
void video_control_get_len()
{
	switch (MSB(gsUsbRequest.wIndex))
	{
		case VC_INTERFACE_CONTROL :
			switch (MSB(gsUsbRequest.wValue))
			{
				case VC_VIDEO_POWER_MODE_CONTROL :
					gbBuffer[0] = 0x01;
					break;
				case VC_REQUEST_ERROR_CODE_CONTROL :
					gbBuffer[0] = 0x01;
					break;
				default :
					ERROR_VIDEO_INVALID_CONTROL();
					break;
			}			
			break;
			
		case VC_CAMERA_TERMINAL :
			switch (MSB(gsUsbRequest.wValue))
			{
				case CT_AE_MODE_CONTROL :
					gbBuffer[0] = 0x01;
					break;
				case CT_EXPOSURE_TIME_ABSOLUTE_CONTROL :
					gbBuffer[0] = 0x04;
					break;
				/*
				case CT_IRIS_ABSOLUTE_CONTROL :
					gbBuffer[0] = 0x02;
					break;
				*/
				case CT_ROLL_ABSOLUTE_CONTROL :
					gbBuffer[0] = 0x02;
					break;
				default :
					ERROR_VIDEO_INVALID_CONTROL();					
					break;
			}			
			break;
		case VC_PROCESSING_UNIT :
			switch (MSB(gsUsbRequest.wValue))
			{
				case PU_BACKLIGHT_COMPENSATION_CONTROL :
					gbBuffer[0] = 0x02;
					break;
				case PU_BRIGHTNESS_CONTROL :	 //len
					gbBuffer[0] = 0x02;
					break;
				case PU_CONTRAST_CONTROL :
					gbBuffer[0] = 0x02;
					break;
				case PU_GAIN_CONTROL :
					gbBuffer[0] = 0x02;
					break;
				case PU_POWER_LINE_FREQUENCY_CONTROL :
					gbBuffer[0] = 0x01;
					break;
				case PU_HUE_CONTROL :	//len
					gbBuffer[0] = 0x02;
					break;
				case PU_SATURATION_CONTROL :
					gbBuffer[0] = 0x02;
					break;
				case PU_SHARPNESS_CONTROL :
					gbBuffer[0] = 0x02;
					break;
				case PU_GAMMA_CONTROL :
					gbBuffer[0] = 0x02;
					break;
				case PU_WHITE_BALANCE_TEMPERATURE_CONTROL:
					gbBuffer[0] = 0x02;
					break;
				case PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL:
					gbBuffer[0] = 0x01;
					break;
				default :
					ERROR_VIDEO_INVALID_CONTROL();					
					break;
			}
			break;

		//test -------------------------------
//		case VC_OUTPUT_TERMINAL:
//			gbBuffer[0] = 0x01;
//			break;

///		case VC_SELECTOR_UNIT:
//			gbBuffer[0] = 0x01;
//			break;
		// end test__________________________________


			
		default :
			ERROR_VIDEO_INVALID_UNIT();
			break;
	}

	gbBuffer[1] = 0x00;
	if (!gbVideoError)
		usb_setup_in(2);
}

extern void setInterfaceStop (void);
extern void setInterfaceStart (void);
extern void UVCStop (void);
extern void UVCStart (void);

void video_stream_set_cur()
{
	BYTE xdata* pData;
	BYTE idata i;

	printf ("video_stream_set_cur %x\n", MSB(gsUsbRequest.wValue));
	switch (MSB(gsUsbRequest.wValue))
	{
		case VS_PROBE_CONTROL :			//进行修改
			usb_fetch_out(26);
			pData = (BYTE xdata*)&gsVideoProbeCtrl;
			for (i = 0; i < 26; i++)
				pData[i] = gbBuffer[i];
			
			/*printf ("video_stream_set_cur VS_PROBE_CONTROL\n");
			printf ("bmHint=%x, bFormatIndex=%d, bFrameIndex=%d\n", gsVideoProbeCtrl.bmHint, 
						gsVideoProbeCtrl.bFormatIndex, gsVideoProbeCtrl.bFrameIndex);
			printf ("dwMaxVideoFrameSize=%d\n",gsVideoProbeCtrl.dwMaxVideoFrameSize);
			printf ("dwMaxPayloadTransferSize=%d\n", gsVideoProbeCtrl.dwMaxPayloadTransferSize);*/

		//	setInterfaceStop();
		//	UVCStop ();			
			break;
		case VS_COMMIT_CONTROL :
			usb_fetch_out(26);

			pData = (BYTE xdata*)&gsVideoProbeCtrl;
			for (i = 0; i < 26; i++)
				pData[i] = gbBuffer[i];
			
			/*printf ("video_stream_set_cur VS_COMMIT_CONTROL\n");
			printf ("bmHint=%x, bFormatIndex=%d, bFrameIndex=%d\n", gsVideoProbeCtrl.bmHint, 
						gsVideoProbeCtrl.bFormatIndex, gsVideoProbeCtrl.bFrameIndex);
			printf ("dwMaxVideoFrameSize=%d\n",gsVideoProbeCtrl.dwMaxVideoFrameSize);
			printf ("dwMaxPayloadTransferSize=%d\n", gsVideoProbeCtrl.dwMaxPayloadTransferSize);*/

#if _PAYLOAD_PIPE_ == _PAYLOAD_PIPE_BULK_
			//UVCStop ();
			//setInterfaceStop ();
			setInterfaceStart();
			UVCStart ();
#endif
			break;
		case VS_STILL_PROBE_CONTROL :	//进行修改
			usb_fetch_out(11);
			pData = (BYTE xdata*)&gsStillProbeCtrl;
			for (i = 0; i < 11; i++)
				pData[i] = gbBuffer[i];

			gsStillProbeCtrl.dwMaxVideoFrameSize = gsVideoProbeCtrl.dwMaxVideoFrameSize;
			gsStillProbeCtrl.dwMaxPayloadTransferSize = gsVideoProbeCtrl.dwMaxPayloadTransferSize;

			break;
		case VS_STILL_COMMIT_CONTROL :
			usb_fetch_out(11);
			pData = (BYTE xdata*)&gsStillProbeCtrl;
			for (i = 0; i < 11; i++)
				pData[i] = gbBuffer[i];
			
			if(gsVideoProbeCtrl.bFormatIndex == gsStillProbeCtrl.bFormatIndex)
			{
				bFlg_needFormatChange = 0;	//yuv video -> yuv image or mjpeg video -> mjpeg image
			}
			else if((gsVideoProbeCtrl.bFormatIndex == 0x01) && (gsStillProbeCtrl.bFormatIndex == 0x02))
			{
				bFlg_needFormatChange = 1;	//yuv video -> mjpeg image
			}
			else if((gsVideoProbeCtrl.bFormatIndex == 0x02) && (gsStillProbeCtrl.bFormatIndex == 0x01))
			{
				bFlg_needFormatChange = 2;	//mjpeg video -> yuv image
			}

			break;
		case VS_STILL_IMAGE_TRIGGER_CONTROL :
			usb_fetch_out(1);
			switch (gbBuffer[0])
			{
				case 0x00 :	//Normal Operation
					gsVideoStatus.bCaptureHeadFig = FALSE;
					break;
				case 0x01 :	//Transfer still image
					gsVideoStatus.bCaptureHeadFig = TRUE;
					gsVideoStatus.bCapture ^= 0x01;
					break;
				case 0x02 :	//Transfer still image via bulk pipe
					ERROR_VIDEO_OUT_RANGE();
					break;
				case 0x03 :	//Abort still image transmission
					gsVideoStatus.bCaptureHeadFig = FALSE;
					break;
				default :
					ERROR_VIDEO_OUT_RANGE();
					break;
			}
			break;
		default :
			ERROR_VIDEO_INVALID_CONTROL();
			break;
	}
}

void video_stream_get_cur()
{
	BYTE xdata* pData;
	BYTE idata i;

	// printf ("video_stream_get_cur %x\n", MSB(gsUsbRequest.wValue));
	switch (MSB(gsUsbRequest.wValue))
	{
		case VS_PROBE_CONTROL :
			(*pFunc_Sensor_Sel_AltSet)();
			pData = (BYTE xdata*)&gsVideoProbeCtrl;
			for (i = 0; i < 26; i++)
				gbBuffer[i] = pData[i];
			/*printf ("video_stream_get_cur VS_PROBE_CONTROL\n");
			printf ("dwMaxVideoFrameSize=%x\n", gsVideoProbeCtrl.dwMaxVideoFrameSize);
			printf ("dwMaxPayloadTransferSize=%x\n", gsVideoProbeCtrl.dwMaxPayloadTransferSize);*/
			usb_setup_in(26);
			break;
		case VS_COMMIT_CONTROL :
			(*pFunc_Sensor_Sel_AltSet)();
			pData = (BYTE xdata*)&gsVideoProbeCtrl;
			for (i = 0; i < 26; i++)
				gbBuffer[i] = pData[i];
			/*printf ("video_stream_get_cur VS_COMMIT_CONTROL\n");
			printf ("dwMaxVideoFrameSize=%x\n", gsVideoProbeCtrl.dwMaxVideoFrameSize);
			printf ("dwMaxPayloadTransferSize=%x\n", gsVideoProbeCtrl.dwMaxPayloadTransferSize);*/
			usb_setup_in(26);
			break;
		case VS_STILL_PROBE_CONTROL :
			(*pFunc_Sensor_Sel_AltSet)();
			pData = (BYTE xdata*)&gsStillProbeCtrl;
			for (i = 0; i < 11; i++)
				gbBuffer[i] = pData[i];
			usb_setup_in(11);
			break;
		case VS_STILL_COMMIT_CONTROL :
			pData = (BYTE xdata*)&gsStillProbeCtrl;
			for (i = 0; i < 11; i++)
				gbBuffer[i] = pData[i];
			usb_setup_in(11);
			break;
		case VS_STILL_IMAGE_TRIGGER_CONTROL :
			gbBuffer[0] = gsVideoStatus.bCapture;
			usb_setup_in(1);
			break;
		default :
			ERROR_VIDEO_INVALID_CONTROL();
			break;
	}
}
