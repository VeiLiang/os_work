#ifndef _SENSORLIB_H
#define _SENSORLIB_H

extern xdata void (*pFunc_GpioInitProcess)();
extern xdata void (*pFunc_VideoAttrAdj)();
extern xdata void (*pFunc_AudioInit)();
extern xdata void (*pFunc_pFunc_AudioCtrl_SetCur)();
extern xdata void (*pFunc_pFunc_AudioEpCtrl_SetCur)();

extern xdata void (*pFunc_UsbSetInterface)();

#if MASK_VER_2
extern xdata void (*pFunc_setInterfaceStart)();
extern xdata void (*pFunc_setInterfaceStop)();
#endif

extern void (*pFunc_Sensor_Sel_AltSet)();
extern void (*pFunc_SensorSetChannelGain)();
extern void (*pFunc_SensorSetShutter)();
extern void (*pFunc_SensorSetGain)();
extern idata void (*pFunc_SensorSetDspWin)();
extern void (*pFunc_SensorSetAttributes)();



extern xdata void (*pFunc_usb_endpoints_reset)();
extern xdata void (*pFunc_usb_resume)();
extern xdata void (*pFunc_usb_reset)();
extern xdata void (*pFunc_usb_suspend)();

BOOL sensor_lib_init();


#endif
