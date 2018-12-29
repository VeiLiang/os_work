#ifndef SENSORCOMMON_H
#define SENSORCOMMON_H

extern BYTE code common_yuv_winFrame_order[15];
extern DWORD code common_yuv_winFrame_BufferSize[14];
extern WORD code common_yuv_wFrameSize_width[14];
extern WORD code common_yuv_wFrameSize_height[14];

void set_dsp_default();
void rerume_to_default_effect();
void set_to_red_effect();
void set_to_green_effect();
void set_to_blue_effect();
void set_to_orange_effect();
void set_to_BLACK_WHITE_effect();
void set_to_CHROMA_REVERSE_effect();
void set_to_LUMINANCE_REVERSE_effect();
void Sensor_FsQvga_AltSet();
void Sensor_FsCif_AltSet();
void Sensor_FsOtherWin_AltSet();
void Sensor_HsQcif_AltSet();
void Sensor_HsOtherWin_AltSet();

void get_Jpeg_CurJpegSize(BYTE bFormat);

void Sensor_Common_Sel_AltSet();
void Sensor_Common_OverFlow_handle();
void Sensor_Default_Set_DspWindow();

#if MASK_VER_2



void setInterfaceStart();
void setInterfaceStop();
#endif

#endif
