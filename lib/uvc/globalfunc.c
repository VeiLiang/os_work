#include <xm_proj_define.h>
#include <string.h>
#include "common.h"
#include "sensorlib.h"
#include "SensorCommon.h"
#include "gpio_function.h"
#include "requests.h"


void Delay(WORD a)
{
	WORD i;
	while(a--)
	{
		for(i = 0;i < 100;i++)
		{;}
	}
}



void dummy_sensor_function()
{
	return;
}

void dummy_sensor_function2(BYTE bValue)
{
	bValue = bValue;
}

void initSetFlicker()
{
	if(gsDeviceStatus.bDeflickMode == DEFLICK_MODE_50Hz)	       // Set to 50Hz
	{
	}
	else if(gsDeviceStatus.bDeflickMode == DEFLICK_MODE_60Hz)	   // Set to 60Hz
	{
	}
	else                       // Out door
	{
		gsDeviceStatus.bDeflickMode = DEFLICK_MODE_OUTDOOR;		
	}
}



//���еĳ�ʼ��ֵ����ο�E2������GPIO״̬�Լ�USB����״̬
//��Ҵ���ʱ����ֱ����MAIN_EXT��ʵ��
void device_init()
{
	disable_usb_int();	

	//��Һ����ӿڣ���Ҫ��ϸ����
	// Init the Sensor function point
	pFunc_Sensor_Sel_AltSet		= Sensor_Common_Sel_AltSet;	//dummy_sensor_function;
	pFunc_SensorSetShutter 		= dummy_sensor_function;
	pFunc_SensorSetChannelGain 	= dummy_sensor_function;
	pFunc_SensorSetGain			= dummy_sensor_function;
	pFunc_SensorSetDspWin			= Sensor_Default_Set_DspWindow; //dummy_sensor_function;
	pFunc_SensorSetAttributes	= dummy_sensor_function;
	pFunc_GpioInitProcess			= gpio_function_init;
	pFunc_AudioInit				= dummy_sensor_function;
	pFunc_pFunc_AudioCtrl_SetCur	= audio_control_set_cur;
	pFunc_pFunc_AudioEpCtrl_SetCur	= audio_endpoint_control_set_cur;
	pFunc_UsbSetInterface			= UsbSetInterface;
#if MASK_VER_2	
	pFunc_setInterfaceStart		= setInterfaceStart;
	pFunc_setInterfaceStop		= setInterfaceStop;
#endif
	pFunc_usb_endpoints_reset	= usb_endpoints_reset;
	pFunc_usb_resume				= usb_resume;
	pFunc_usb_reset				= usb_reset;
	pFunc_usb_suspend				= usb_suspend;

	//gsDeviceStatus.bUsb_HighSpeed = TRUE;	//�ó�ʼ��USBΪHS���
	gsDeviceStatus.bUsb_DevState = DEVSTATE_DEFAULT;
	//gsDeviceStatus.bKeySel = 1;
	gsDeviceStatus.bDeflickMode = 1;
	gsDeviceStatus.bUsbHsOrFs = 1;
	gsDeviceStatus.bMicSel = 0;
	gsDeviceStatus.bYuvOrRgb = 1;
	gsDeviceStatus.bFormatSel = 1;
	gsDeviceStatus.bDualFormatSel = 1;
	gsDeviceStatus.bDualSensorSel = 1;

	bDebugFlg = 0;
	
	//(*pFunc_GpioInitProcess)();	//gpio_init();

	(*pFunc_GpioInitProcess)();	//gpio_init();

	//updateGpioMapTbl();		//����E2�����滻֮ǰ��GPIO��ʼ��

	readVideoWinIndex();	//Init video Probe control

	readStillWinIndex();	//Init still image Probe control
	
	bmAttrAdjust = 0x0000;	//0xFFFF;
	gsVideoStatus.bmAdjust = 0xFFFF;//0x0000;	//0xFFFF;	//Updata all default values
	gsVideoStatus.bStartVideo = FALSE;
	gsVideoStatus.bCapture = 0xac;
	gsVideoStatus.bCaptureHeadFig = FALSE;
	//gsVideoStatus.bCaptureFig = FALSE;
	gsVideoStatus.bEffectFig = FALSE;
	gsVideoStatus.bEffectIdx = 0x00;
	gsVideoStatus.bEffectCount = 0x0b;

	//��������Ĭ��ֵ
	gsVideoDefault.bAEMode = 2;	// 4 - CAM AE auto disable; 2 - CAE AE auto enable
	gsVideoDefault.wExposureTime = 0x000c;
	gsVideoDefault.bRoll = 0;
	gsVideoDefault.bBacklightComp = 1;
	gsVideoDefault.bBrightness = 128;
	gsVideoDefault.bContrast = 128;
	gsVideoDefault.wGain = 0x0010;
	gsVideoDefault.bPowerLineFreq = 0x01;
	gsVideoDefault.bFlickerFlag = 0x01;
	gsVideoDefault.wHue = 0;
	//gsVideoDefault.wHue = 128;
	gsVideoDefault.bSaturation = 64;
	gsVideoDefault.bSharpness = 1;
	//gsVideoDefault.bGamma = 4;
	gsVideoDefault.bGamma = 4;
	gsVideoDefault.wWBC = 6500;
	gsVideoDefault.bWBAuto = 1;	//  1- video WB auto enable; 0 - video WB auto disable
	
	gsVideoLimit.wMaxExposureTime = 0x03E9;
	gsVideoLimit.bMinBacklightComp = 1;
	gsVideoLimit.bMaxBacklightComp = 5;
	gsVideoLimit.bMaxPowerLineFreq = 2;
	gsVideoLimit.bMaxSharpness = 255;
	gsVideoLimit.bMinGamma = 1;
	gsVideoLimit.bMaxGamma = 8;
	//gsVideoLimit.bMaxGamma = 5;
	
	//audio init
	update_audio_cur_attribute();

	//usb init
	reg_usb_Index = 0; 				//ep0
	// reg_usb_IntrInE = 0x0D;		//ep0,ep2,ep3 IN�ж�ʹ��
	// reg_usb_IntrInE = 0x25;		//ep0,ep2,ep5 IN�ж�ʹ�� 00100101
	reg_usb_IntrInE = 0x5 | (1 << VIDEO_EP_INDEX);		//ep0,ep2,ep5 IN�ж�ʹ�� 00100101
	reg_usb_IntrOutE = 0x01;	//ep1 out�ж�ʹ��
	reg_usb_IntrUSBE = 0x07;	//usb reset/resume/suspend�ж�ʹ��

	MGC_Write8(MUSB_IDX_TXCSRH, MGC_Read8(MUSB_IDX_TXCSRH)|TXCSRH_FrcDataTog);

	//ep0 init
	gsEp0Status.bState = EP0_IDLE;
	gsEp0Status.wBytesLeft = 0;
	gsEp0Status.bTestMode = 0;
	gsEp0Status.bFAddr = 0xFF;
	
	//���ж�
	enable_usb_int();
}

//#define	YUV422
//��ȡ��Ƶ������Ϣ
void readVideoWinIndex()
{
	{
		gsVideoProbeCtrl.bmHint = 0x0000;

		gsVideoProbeCtrl.bFormatIndex = 0x01;
		gsVideoProbeCtrl.bFrameIndex = 0x01;
		gsVideoProbeCtrl.dwFrameInterval = B2L_32(0x15160500);
		gsVideoProbeCtrl.wKeyFrameRate = 0x0000;
		gsVideoProbeCtrl.wPFrameRate = 0x0000;
		gsVideoProbeCtrl.wCompQuality = 0x0000;
		gsVideoProbeCtrl.wCompWindowSize = 0x0000;
		gsVideoProbeCtrl.wDelay = 0x0000;
		gsVideoProbeCtrl.dwMaxVideoFrameSize = B2L_32(0x00600900);	//test
		gsVideoProbeCtrl.dwMaxPayloadTransferSize = USB20_MAX_VIDEO_PAYLOAD_SIZE;
	}
}

//��ȡͼ�񴰿���Ϣ
void readStillWinIndex()
{
	{
		gsStillProbeCtrl.bFormatIndex = gsVideoProbeCtrl.bFormatIndex;	//gsStillProbeCtrl.bFormatIndex = 0x01;
		gsStillProbeCtrl.bFrameIndex = 	gsVideoProbeCtrl.bFrameIndex;	//gsStillProbeCtrl.bFrameIndex = 0x01;
		gsStillProbeCtrl.bCompressionIndex = 0x00;
		gsStillProbeCtrl.dwMaxVideoFrameSize = gsVideoProbeCtrl.dwMaxVideoFrameSize;//gsStillProbeCtrl.dwMaxVideoFrameSize = 0x00600900;		 //640x480x2
		gsStillProbeCtrl.dwMaxPayloadTransferSize = gsVideoProbeCtrl.dwMaxPayloadTransferSize;	//gsStillProbeCtrl.dwMaxPayloadTransferSize = USB20_MAX_VIDEO_PAYLOAD_SIZE;
	}	
}

//gsVideoDefault <-- gsVideoStatus
//��Ҵ���ʱ������MAIN_EXT��ֱ��ʵ��
void update_video_cur_attribute()
{
	{
		gsVideoStatus.bAEMode = gsVideoDefault.bAEMode;
		gsVideoStatus.wExposureTime = gsVideoDefault.wExposureTime;
		gsVideoStatus.bRoll = gsVideoDefault.bRoll;
		gsVideoStatus.bBacklightComp = gsVideoDefault.bBacklightComp;
		gsVideoStatus.bBrightness = gsVideoDefault.bBrightness;
		gsVideoStatus.bContrast = gsVideoDefault.bContrast;
		gsVideoStatus.wGain = gsVideoDefault.wGain;
		gsVideoStatus.bPowerLineFreq = gsVideoDefault.bPowerLineFreq;
		gsVideoStatus.bFlickerFlag = gsVideoDefault.bFlickerFlag;
		gsVideoStatus.wHue = gsVideoDefault.wHue;
		gsVideoStatus.bSaturation = gsVideoDefault.bSaturation;
		gsVideoStatus.bSharpness = gsVideoDefault.bSharpness;
		gsVideoStatus.bGamma = gsVideoDefault.bGamma;
		gsVideoStatus.wWBC = gsVideoDefault.wWBC;
		gsVideoStatus.bWBAuto = gsVideoDefault.bWBAuto;
	}
}



void update_audio_cur_attribute()
{
	gsAudioStatus.bStartAudio = FALSE;
	gsAudioStatus.bMute = 0;


	//��ʼ����Ƶ���ȫ�ֱ�������������ָ��audio_depth��inmax��max_vol��iso_size��interval��AUDIO_EN_DELAY
  	AUDIO_SIZE_1_FRAME = 0x60;

	USB_INMAX_L_AUDIO_VAL = 0x60;
	USB_INMAX_H_AUDIO_VAL = 0x00;
	
	MAX_MIC_VOLUME = 0x2fff;
	
	MAX_AUDIO_PAYLOAD_SIZE = 0x6000;
	AUDIO_INTERVAL_HS = 0x0400;
	AUDIO_INTERVAL_FS = 0x0400;
	

	gsAudioLimit.wMaxVolume = MAX_MIC_VOLUME;
}




