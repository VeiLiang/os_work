#include "common.h"
#include "gpio_function.h"




void gpio_function_init()
{
	//YUV or RGB
	gsDeviceStatus.bYuvOrRgb = 1;
	
	//bDebugFlg |= 0x02;

	//MIC sel
	gsDeviceStatus.bMicSel = 0;	// 无MIC
	
	// USB2.0 or 1.0
	if(1)
	{
		reg_usb_Power |= bmBIT5;			//使能hs模式
		gsDeviceStatus.bUsbHsOrFs = 1;
	}
	else
	{
		reg_usb_Power &= (~bmBIT5);
		gsDeviceStatus.bUsbHsOrFs = 0;
	}
		
	//Flicker 50Hz or 60Hz
	if(1)
	{
		gsDeviceStatus.bDeflickMode = DEFLICK_MODE_50Hz;
	}
	else
	{
		gsDeviceStatus.bDeflickMode = DEFLICK_MODE_60Hz;
	}
	
	//FORMAT sel - YUV or JPEG
	//gsDeviceStatus.bFormatSel = 1;		//YUV
	gsDeviceStatus.bFormatSel = 0;		//JPEG
	
	//Dual-Format Sel
	if(1)
	{
		gsDeviceStatus.bDualFormatSel = 1;		//单格式
	}
	else
	{
		gsDeviceStatus.bDualFormatSel = 0;		//双格式
	}

	


}

