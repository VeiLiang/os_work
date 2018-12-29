#include "common.h"
#include "sensorlib.h"
#include "gpio_function.h"
#include "SensorCommon.h"


xdata void (*pFunc_GpioInitProcess)();
xdata void (*pFunc_AudioInit)();
xdata void (*pFunc_pFunc_AudioCtrl_SetCur)();
xdata void (*pFunc_pFunc_AudioEpCtrl_SetCur)();
xdata void (*pFunc_UsbSetInterface)();

#if MASK_VER_2
xdata void (*pFunc_setInterfaceStart)();
xdata void (*pFunc_setInterfaceStop)();
#endif

void (*pFunc_Sensor_Sel_AltSet)();
void (*pFunc_SensorSetChannelGain)();	//AWB
void (*pFunc_SensorSetShutter)();		//AE : time
void (*pFunc_SensorSetGain)();			//AE : amp
idata void (*pFunc_SensorSetDspWin)();		//用于set_interface  中
void (*pFunc_SensorSetAttributes)();	//用于Virtical Mirror  及AE  的使能



xdata void (*pFunc_usb_endpoints_reset)();
xdata void (*pFunc_usb_resume)();
xdata void (*pFunc_usb_reset)();
xdata void (*pFunc_usb_suspend)();

DWORD xdata exposeTime;
WORD xdata exposeGain;

#define ACTIVEHIGHT   	1
#define ACTIVELOW   	2

BYTE idata bRollDegree = 180;
WORD idata AWB_Degree = 1;


BOOL sensor_lib_init()
{
	pFunc_SensorSetDspWin		= Sensor_Default_Set_DspWindow;
	pFunc_Sensor_Sel_AltSet 	= Sensor_Common_Sel_AltSet;
		
	return STATUS_SUCCESS;
}

