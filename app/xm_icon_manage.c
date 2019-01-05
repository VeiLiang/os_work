//****************************************************************************
//
//	Copyright (C) 2010~2014 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_icon_manage.c
//	  状态ICON的管理、显示、隐藏
//
//	Revision history
//
//		2012.09.01	ZhuoYongHong Initial version
//
//****************************************************************************
/*
*/
#include <xm_proj_define.h>
#include <xm_type.h>
#include <string.h>
#include <xm_base.h>
#include <xm_user.h>
#include <xm_gdi.h>
#include <xm_printf.h>
#include <xm_image.h>
#include <xm_heap_malloc.h>
#include <xm_rom.h>
#include <stdlib.h>
#include <stdio.h>
#include <xm_semaphore.h>
#include <xm_h264_codec.h>
#include "xm_icon_manage.h"
#include "app_icon_library.h"
#include "xm_osd_layer.h"
#include "app.h"
#include "rom.h"
#include "types.h"
#include "rxchip.h"


#define	_ICON_24_USE_

#define	DATETIME_CHAR_WIDTH	18
#define	DATETIME_CHAR_HEIGHT 36
#define	DATETIME_CHAR_RED_WIDTH	18

extern unsigned char get_takephoto_flag(void);
extern int CurrentPhotoMode;
extern unsigned int DispDesktopMenu;
extern unsigned int XMSYS_H264CodecGetCurrentVideoRecoringTime (void);
extern struct _rxchip_video g_rxchp_video_param;

typedef struct _tagICONSCHEME {
	unsigned int	icon_scheme;
	unsigned int	icon_type;

	unsigned int	osd_w;		
	unsigned int	osd_h;

	float			x_ratio;
	float			y_ratio;

	unsigned	int	icon_w;		// ICON像素宽度
	unsigned int	icon_h;		// ICON像素高度
	unsigned int	icon_space;	// ICON间距
	unsigned int	horz_space;	// 水平边空白
	unsigned int	vert_space;	// 垂直边空白

	unsigned int	space_width;// 空格字符宽度
} ICONSCHEME;

#define	ICON_SCHEME_1		0
      //     方案1 ICON排列
      //  
      //     录像状态/录像分辨率                                            卡状态 主电池/USB充电标志
      //                   
      //
      // 
      //     MIC/VOICE                     已录时间                         系统时间                

#define	ICON_SCHEME_2		1
      //     方案2 ICON排列

static const unsigned int datetime_char_offset[] = {

	ROM_T18_FONT_S18_DATETIME_01_PNG,
	ROM_T18_FONT_S18_DATETIME_02_PNG,
	ROM_T18_FONT_S18_DATETIME_03_PNG,
	ROM_T18_FONT_S18_DATETIME_04_PNG,
	ROM_T18_FONT_S18_DATETIME_05_PNG,
	ROM_T18_FONT_S18_DATETIME_06_PNG,
	ROM_T18_FONT_S18_DATETIME_07_PNG,
	ROM_T18_FONT_S18_DATETIME_08_PNG,
	ROM_T18_FONT_S18_DATETIME_09_PNG,
	ROM_T18_FONT_S18_DATETIME_10_PNG,
	ROM_T18_FONT_S18_DATETIME_37_PNG,
	ROM_T18_FONT_S18_DATETIME_39_PNG
};

static const unsigned int datetime_char_length[] = {
	ROM_T18_FONT_S18_DATETIME_01_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_02_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_03_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_04_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_05_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_06_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_07_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_08_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_09_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_10_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_37_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_39_PNG_SIZE
};

static const unsigned int datetime_red_char_offset[] = {
	ROM_T18_FONT_S18_DATETIME_CHAR_01_RED_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_02_RED_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_03_RED_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_04_RED_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_05_RED_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_06_RED_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_07_RED_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_08_RED_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_09_RED_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_10_RED_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_37_RED_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_39_RED_PNG
};

static const unsigned int datetime_red_char_length[] = {
	ROM_T18_FONT_S18_DATETIME_CHAR_01_RED_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_02_RED_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_03_RED_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_04_RED_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_05_RED_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_06_RED_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_07_RED_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_08_RED_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_09_RED_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_10_RED_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_37_RED_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_39_RED_PNG_SIZE
};

static const unsigned int s32_white_black_datetime_char_offset[] = {
	ROM_T18_FONT_S32_WHITE_BLACK_0_PNG,
	ROM_T18_FONT_S32_WHITE_BLACK_1_PNG,
	ROM_T18_FONT_S32_WHITE_BLACK_2_PNG,
	ROM_T18_FONT_S32_WHITE_BLACK_3_PNG,
	ROM_T18_FONT_S32_WHITE_BLACK_4_PNG,
	ROM_T18_FONT_S32_WHITE_BLACK_5_PNG,
	ROM_T18_FONT_S32_WHITE_BLACK_6_PNG,
	ROM_T18_FONT_S32_WHITE_BLACK_7_PNG,
	ROM_T18_FONT_S32_WHITE_BLACK_8_PNG,
	ROM_T18_FONT_S32_WHITE_BLACK_9_PNG,
	ROM_T18_FONT_S32_WHITE_BLACK_DOT_PNG,
	ROM_T18_FONT_S32_WHITE_BLACK_SLASH_PNG,
};

static const unsigned int s32_white_black_datetime_char_length[] = {
	ROM_T18_FONT_S32_WHITE_BLACK_0_PNG_SIZE,
	ROM_T18_FONT_S32_WHITE_BLACK_1_PNG_SIZE,
	ROM_T18_FONT_S32_WHITE_BLACK_2_PNG_SIZE,
	ROM_T18_FONT_S32_WHITE_BLACK_3_PNG_SIZE,
	ROM_T18_FONT_S32_WHITE_BLACK_4_PNG_SIZE,
	ROM_T18_FONT_S32_WHITE_BLACK_5_PNG_SIZE,
	ROM_T18_FONT_S32_WHITE_BLACK_6_PNG_SIZE,
	ROM_T18_FONT_S32_WHITE_BLACK_7_PNG_SIZE,
	ROM_T18_FONT_S32_WHITE_BLACK_8_PNG_SIZE,
	ROM_T18_FONT_S32_WHITE_BLACK_9_PNG_SIZE,
	ROM_T18_FONT_S32_WHITE_BLACK_DOT_PNG_SIZE,
	ROM_T18_FONT_S32_WHITE_BLACK_SLASH_PNG_SIZE,
};

static unsigned int icon_state[AP_ICON_COUNT];		// icon状态值
static XM_IMAGE *icon_image[AP_ICON_COUNT];			// icon图像对象

static XM_IMAGE *flag_image = NULL;						// 标志图像对象

// 保存0123456789:/字符的对象
static XM_IMAGE *image_datetime[12];

// 保存0123456789:/字符的对象,红色
static XM_IMAGE *image_datetime_red[12];

// 描边 0123456789:/字符
static XM_IMAGE *image_datatime_32[12];

static void *icon_semaphore = NULL;						// 线程互斥访问保护信号量

static ICONSCHEME icon_scheme;

XMSYSTEMTIME rendervideotime;              //当前视频文件录制时间
unsigned int rendervideo_start_ticket;		//当前视频文件已回放时间(减去开始时间)
DWORD dispyear,dispmonth,dispday,disphour,dispminute, dispsecond; //当前视频文件显示时间


static int verify_icon_state (unsigned int dwIcon, unsigned int dwIconState)
{
	if(dwIcon >= AP_ICON_COUNT)
	{
		return 0;
	}
	
	switch (dwIcon)
	{
		case AP_ICON_TAKEPHOTO:
			break;
			
		case AP_ICON_MODE:
			break;
			
		case AP_ICON_MIC:
			if(dwIconState >= AP_SETTING_MIC_OPTION_COUNT)
				return 0;
			break;

		case AP_ICON_VOICE:
			if(dwIconState >= AP_SETTING_VOICE_PROMPTS_OPTION_COUNT)
				return 0;
			break;

		case AP_ICON_REC_RED_DOT:
			break;

		case AP_ICON_VIDEORESOLUTION:
			if(dwIconState >= AP_SETTING_VIDEORESOLUTION_COUNT)
				return 0;
			break;

		case AP_ICON_CARD:
			if(dwIconState > DEVCAP_SDCARDSTATE_INVALID)
				return 0;
			break;

		case AP_ICON_USB:
			if(dwIconState > DEVCAP_USB_CONNECT_CAMERA)
				return 0;
			break;

		case AP_ICON_MAIN_BATTERY:
			if(dwIconState > DEVCAP_VOLTAGE_GOOD)
				return 0;
			break;

		case AP_ICON_BACKUP_BATTERY:
			if(dwIconState > DEVCAP_VOLTAGE_GOOD)
				return 0;
			break;
		
		case AP_ICON_GPS:
			if(dwIconState > DEVCAP_GPSBD_CONNECT_LOCATE_OK)
				return 0;
			break;

        case AP_ICON_PHOTO:
			break;
	    case AP_ICON_V_LINE:
            break;
	    case AP_ICON_H_LINE:
            break;
        case AP_ICON_CH_AHD1:
            break;
        case AP_ICON_CH_AHD2:
            break;
        case AP_ICON_BIAOZHI:
            break;
        case AP_ICON_HOME:
			break;

		case AP_ICON_SYS_SETTING:
			break;

		case AP_ICON_VIDEO_SWITCH:
			break;

		case AP_ICON_PHOTOGRAPH:
			break;

		case AP_ICON_RECORD_SWITCH:
			break;

		case AP_ICON_RECORD_LOCK:
			break;
        case AP_ICON_NO_SIGNED:
            break;
		case AP_ICON_RECORD_LIST:
			break;
			
		case AP_ICON_LOCK:
			if(dwIconState >= AP_SETTING_VIDEOLOCK_OPTION_COUNT)
				return 0;
			
		case AP_ICON_DAY_NIGHT_MODE:
			if(dwIconState >= AP_SETTING_DAY_NIGHT_MODE_OPTION_COUNT)
				return 0;	
			  break;
		default:
			return 0;
	}
	return 1;
}

int Pre_Signal_Data = 0;
//int Pre_Blue_Data = 0;
extern BOOL Camera_Data_signed;

static APPROMRES *GetIconResource (unsigned int dwIconType, unsigned int dwIcon, unsigned int dwIconState)
{
	unsigned int dwIconResource;
	if(verify_icon_state (dwIcon, dwIconState) == 0)
	{
		XM_printf ("invalid icon(%d)'s state(%d)\n", dwIcon, dwIconState);
		return NULL;
	}
	
	switch (dwIcon)
	{
		case AP_ICON_TAKEPHOTO:
			dwIconResource = AP_ID_ICON_TAKEPHOTO;
			break;
			
		case AP_ICON_MODE:
			dwIconResource = AP_ID_ICON_MODE;
			break;
			
		case AP_ICON_MIC:
			if(dwIconState == AP_SETTING_MIC_OFF)
				dwIconResource = AP_ID_ICON_MIC_OFF;
			else
				dwIconResource = AP_ID_ICON_MIC_ON;
			break;

		case AP_ICON_VOICE:
			if(dwIconState == AP_SETTING_VOICE_PROMPTS_OFF)
				dwIconResource = AP_ID_ICON_VOLUME_MUTE;
			else
				dwIconResource = AP_ID_ICON_VOLUME_ON;
			break;

		case AP_ICON_REC_RED_DOT:
			dwIconResource = AP_ID_ICON_VIDEO_REC_RED_DOT;
			break;

		case AP_ICON_CARD:
			if(dwIconState != DEVCAP_SDCARDSTATE_UNPLUG)
				dwIconResource = AP_ID_ICON_SDCARD;
			else
				dwIconResource = AP_ID_ICON_SDCARD_PLUGIN;
			
			break;

		case AP_ICON_USB:
			if(dwIconState == DEVCAP_USB_DISCONNECT)
				dwIconResource = AP_ID_ICON_USB_DISCONNECTED;
			else if(dwIconState == DEVCAP_USB_CONNECT_CHARGE)
				dwIconResource = AP_ID_ICON_BATTERY_CHARGE;
			else if(dwIconState == DEVCAP_USB_CONNECT_UDISK)
				dwIconResource = AP_ID_ICON_USB_DISK;
			else
				dwIconResource = AP_ID_ICON_USB_CONNECTED;
			break;

		case AP_ICON_MAIN_BATTERY:
			if(dwIconState == DEVCAP_BATTERYVOLTAGE_WORST)
				dwIconResource = AP_ID_ICON_BATTERY_WORST;
			else if(dwIconState == DEVCAP_BATTERYVOLTAGE_BAD)
				dwIconResource = AP_ID_ICON_BATTERY_BAD;
			else if(dwIconState == DEVCAP_BATTERYVOLTAGE_NORMAL)
				dwIconResource = AP_ID_ICON_BATTERY_NORMAL;
			else // if(dwIconState == DEVCAP_BATTERYVOLTAGE_BEST)
				dwIconResource = AP_ID_ICON_BATTERY_GOOD;
			break;

		case AP_ICON_GPS:
			if(dwIconState == DEVCAP_GPSBD_CONNECT_LOCATE_OK)	// 定位中
				dwIconResource = AP_ID_ICON_GPS_RECEIVING;
			else //if(dwIconState == DEVCAP_GPSBD_CONNECT_LOCATE_OK)	// GPS断开
				dwIconResource = AP_ID_ICON_GPS_DISCONNECTED;
			break;
			
        case AP_ICON_PHOTO:
			 if(dwIconState==AP_SETTING_PHOTO)
			  	dwIconResource = AP_ID_ICON_PHOTO;
			 else
			  	dwIconResource=AP_ID_ICON_PHOTO_PRESS;
			  break;
			  
        case  AP_ICON_V_LINE:
			dwIconResource = AP_ID_ICON_V_LINE;
			break;

        case  AP_ICON_H_LINE:
			dwIconResource = AP_ID_ICON_H_LINE;
			break;
	
		case AP_ICON_CH_AHD1:
			if(dwIconState==AP_ICON_DISPLAY_ON)
			{
				dwIconResource = AP_ID_ICON_CAMERA1;
			}
			break;
			
		case AP_ICON_CH_AHD2:
			if(dwIconState==AP_ICON_DISPLAY_ON)
			{
				dwIconResource = AP_ID_ICON_CAMERA2;
			}
			break;
			
        case AP_ICON_NO_SIGNED:
            if(Pre_Signal_Data) {
                dwIconResource = AP_ID_ICON_NOSIGNED;
            }else {
                dwIconResource = AP_ID_ICON_NOSIGNED_C;
            }
            break;
			  
	    case AP_ICON_HOME:
			  dwIconResource = AP_ID_ICON_HOME;
			  break;
				
		case AP_ICON_LOCK:
              if(dwIconState==AP_SETTING_VIDEOLOCK_OFF)
              	dwIconResource = AP_ID_ICON_UNLOCK;
              else
              	dwIconResource = AP_ID_ICON_LOCK;	
              break;
				
		case AP_ICON_DAY_NIGHT_MODE:
              if(dwIconState==AP_SETTING_DAY_MODE)
              	dwIconResource = AP_ID_ICON_DAY;
              else
              	dwIconResource = AP_ID_ICON_NIGHT;	
              break;

		case AP_ICON_SYS_SETTING:
			 if(dwIconState==AP_SYS_SETTING)
			  	dwIconResource = AP_ID_ICON_SYS_SETTING;
			 else
			  	dwIconResource=AP_ID_ICON_SYS_SETTING_PRESS;
			  break;

		case AP_ICON_VIDEO_SWITCH:
			 if(dwIconState==AP_VIDEO_SWITCH)
			  	dwIconResource = AP_ID_ICON_VIDEO_SWITCH;
			 else
			  	dwIconResource=AP_ID_ICON_VIDEO_SWITCH_PRESS;
			  break;

		case AP_ICON_PHOTOGRAPH:
			 if(dwIconState==AP_PHOTOGRAPH)
			  	dwIconResource = AP_ID_ICON_PHOTOGRAPH;
			 else
			  	dwIconResource=AP_ID_ICON_PHOTOGRA_PRESS;
			  break;

		case AP_ICON_RECORD_SWITCH:
			 if(dwIconState==AP_RECORD_SWITCH)
			  	dwIconResource = AP_ID_ICON_RECORD_PRESS;
			 else
			  	dwIconResource=AP_ID_ICON_RECORD;
			  break;

		case AP_ICON_RECORD_LOCK:
			 if(dwIconState==AP_RECORD_LOCK)
			  	dwIconResource = AP_ID_ICON_RECORD_LOCK_PRESS;
			 else
			  	dwIconResource=AP_ID_ICON_RECORD_LOCK;
			  break;

		case AP_ICON_RECORD_LIST:
			 if(dwIconState==AP_RECORD_LIST)
			  	dwIconResource = AP_ID_ICON_RECORD_LIST;
			 else
			  	dwIconResource=AP_ID_ICON_RECORD_LIST_PRESS;
			  break;
			  
		default:
			return NULL;
	}

	return AP_AppGetIconResource (dwIconType, dwIconResource);
}

static void load_image (unsigned int dwIcon, unsigned int dwIconState)
{
	APPROMRES *AppRes;
	// 检查是否需要释放老的设置资源
	if(icon_image[dwIcon])
	{
		XM_ImageDelete (icon_image[dwIcon]);
		icon_image[dwIcon] = NULL;
	}

	AppRes = GetIconResource (icon_scheme.icon_type, dwIcon, dwIconState);
	if(AppRes)
	{
		// 创建资源
		icon_image[dwIcon] = XM_RomImageCreate (
							AppRes->rom_offset, 
							AppRes->res_length,
							XM_OSD_LAYER_FORMAT_ARGB888);
		if(icon_image[dwIcon] == NULL)
		{
			XM_printf ("XM_ImageCreate NG, Icon=%d, state=%d\n", dwIcon, dwIconState);
		}
	}
	else
	{
		// XM_printf ("Icon=%d, state=%d resource NG\n", dwIcon, dwIconState);
	}
}

void XM_VideoIconInit (void)
{
	int i;
	// 加载视频内嵌时间戳符号资源
	for (i = 0; i < 12; i++)
	{
		image_datatime_32[i] = XM_RomImageCreate (
							//s32_white_black_datetime_char_offset[i], 
							//s32_white_black_datetime_char_length[i], 
							datetime_char_offset[i],
							datetime_char_length[i],
							XM_OSD_LAYER_FORMAT_ARGB888);
	}
}

void XM_IconInit (void)
{
	int i;
	for (i = 0; i < AP_ICON_COUNT; i ++)
	{
		icon_image[i] = NULL;
		icon_state[i] = 0;
	}

	flag_image = NULL;

	// 根据OSD0的显示尺寸定义ICON的显示策略
	icon_scheme.osd_w = XM_lcdc_osd_get_width (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
	icon_scheme.osd_h = XM_lcdc_osd_get_height (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
	
	//if(icon_scheme.osd_w >= 640 && icon_scheme.osd_h >= 480)
	if((icon_scheme.osd_w >= 640) && (icon_scheme.osd_h >= 400))
	{
		icon_scheme.icon_type = AP_ICON_SIZE_32;
		icon_scheme.icon_w = 32;
		icon_scheme.icon_h = 36;
		icon_scheme.icon_space = 2;
		icon_scheme.horz_space = 20;
		icon_scheme.vert_space = 20;

		// 时间串显示使用的空格字符
		icon_scheme.space_width = DATETIME_CHAR_WIDTH;
	}
	else
	{
#ifdef _ICON_24_USE_
		icon_scheme.icon_type = AP_ICON_SIZE_24;
		icon_scheme.icon_w = 24;
		icon_scheme.icon_h = 24;
#else
		icon_scheme.icon_type = AP_ICON_SIZE_16;
		icon_scheme.icon_w = 16;
		icon_scheme.icon_h = 16;
#endif
		icon_scheme.icon_space = 3;
		icon_scheme.horz_space = 3;
		icon_scheme.vert_space = 3;

		// 时间串显示使用的空格字符
		icon_scheme.space_width = DATETIME_CHAR_WIDTH/2;
	}
	icon_scheme.x_ratio = icon_scheme.osd_w / 320.0f;
	icon_scheme.y_ratio = icon_scheme.osd_h / 240.0f;

	for (i = 0; i < 12; i++)
	{
		image_datetime[i] = XM_RomImageCreate (
							datetime_char_offset[i], 
							datetime_char_length[i],
							XM_OSD_LAYER_FORMAT_ARGB888);
	}

	//红色数字
	for (i = 0; i < 12; i++)
	{
		image_datetime_red[i] = XM_RomImageCreate (
							datetime_red_char_offset[i], 
							datetime_red_char_length[i],
							XM_OSD_LAYER_FORMAT_ARGB888);
	}
	
	icon_semaphore = 	XM_CreateSemaphore ("icon_semaphore", 1);
	if(icon_semaphore == NULL)
	{
		XM_printf ("CreateSemaphore \"icon_semaphore\" NG\n");
	}
}

void XM_IconExit (void)
{
	int i;
	if(icon_semaphore)
	{
		// 关闭信号灯
		XM_CloseSemaphore (icon_semaphore);
		// 删除信号灯
		XM_DeleteSemaphore (icon_semaphore);
		icon_semaphore = NULL;
	}

	for (i = 0; i < AP_ICON_COUNT; i ++)
	{
		if(icon_image[i])
		{
			XM_ImageDelete (icon_image[i]);
			icon_image[i] = NULL;
		}
	}
	/*
	for (i = 0; i < 12; i++)
	{
		if(image_datetime[i])
		{
			XM_ImageDelete (image_datetime[i]);
			image_datetime[i] = NULL;
		}
	}
	*/
	
	if(flag_image)
	{
		XM_ImageDelete (flag_image);
		flag_image = NULL;
	}

}

void XM_VideoIconExit (void)
{
	int i;
	for (i = 0; i < 12; i++)
	{
		if(image_datatime_32[i])
		{
			XM_ImageDelete (image_datatime_32[i]);
			image_datatime_32[i] = NULL;
		}
	}	
}


int XM_ImageBlend_argb888_to_yuv420 (
						XM_IMAGE *lpImage,
						unsigned int img_offset_x,					// 复制区域在源图像的偏移(相对源图像平面的左上角)
						unsigned int img_offset_y,
						unsigned int img_w,							// 待复制区域的像素宽度、像素高度		
						unsigned int img_h,

						unsigned char *osd_layer_buffer,			// 目标OSD的视频数据缓冲区
						unsigned int osd_layer_format,			// 目标OSD的视频格式
						unsigned int osd_layer_width,				// 目标OSD的像素宽度、像素高度
						unsigned int osd_layer_height,	
						unsigned int osd_layer_stride,			// 目标OSD的行字节长度
																			//		YUV420格式表示Y分量，UV分量除以2
						unsigned int osd_offset_x,					//	复制区域在OSD层的偏移 (相对OSD平面的左上角)
						unsigned int osd_offset_y

						);


extern void dma_flush_range(UINT32 ulStart, UINT32 ulEnd);

// 嵌入本地时间戳信息到视频中, 仅支持Y_UV420格式
void embed_local_time_into_video_frame (
										 unsigned int width,			// 视频宽度
										 unsigned int height,		// 视频高度
										 unsigned int stride,	// Y_UV行字节长度
										 char *data					// Y_UV缓冲区
										)
{
	XM_IMAGE *image;
	XMSYSTEMTIME local_time;
	char str[32];
	int x;			// 相对于视频左上角(原点)的偏移
	int y;
	
	int d_x, d_y;
	int d_w, d_h;
	int ret;
	
	int i, count;
	int space_width = 2;
	int font_width;		// 描边字符的宽度
	
	if(image_datatime_32[0] == NULL)
		return;
	
	font_width = image_datatime_32[0]->width;
	
	XM_GetLocalTime (&local_time);

	sprintf (str, "%4d/%02d/%02d %02d:%02d:%02d", local_time.wYear, local_time.bMonth, local_time.bDay, local_time.bHour, local_time.bMinute, local_time.bSecond);
	count = strlen (str);
	
	if(width == 1920 && height == 1080)
	{
		x = 1470;
		y = 32;
	}
	else if(width == 1280 && height == 720)
	{
		x = 950;
		y = 580;		
	}
	else
	{
		return;
	}
	
	d_x = x;
	d_y = y;
	d_w = image_datatime_32[0]->width * count;
	d_h = image_datatime_32[0]->height;
	
	XM_lcdc_inv_region ((unsigned char *)data, 
										 XM_OSD_LAYER_FORMAT_Y_UV420,
										 width,
										 height,
										 width,
										 d_x, d_y,
										 d_w, d_h
										);
	
	
	for (i = 0; i < count; i++)
	{
		if (str[i] >= '0' && str[i] <= '9')
		{
			image = image_datatime_32[str[i] - '0'];	
		}
		else if(str[i] == ':')
			image = image_datatime_32[10];
		else if(str[i] == '/')
			image = image_datatime_32[11];
		else
			image = NULL;
		if(image)
		{
			XM_ImageBlend_argb888_to_yuv420_normal (
					image,
					0,											// 复制区域在源图像的偏移(相对源图像平面的左上角)
					0,
					image->width, 							// 待复制区域的像素宽度、像素高度		
					image->height,
					(unsigned char *)data,			// 目标OSD的视频数据缓冲区
					XM_OSD_LAYER_FORMAT_Y_UV420,		// 目标OSD的视频格式
					width, height,							// 目标OSD的像素宽度、像素高度
					stride,								// 目标OSD的行字节长度
					x, y 										//	复制区域在OSD层的偏移 (相对OSD平面的左上角)
					);								
			
			
			x += font_width;
		}
		else
		{
			x += font_width;	//space_width;
		}
	}
		
	ret = XM_lcdc_flush_dirty_region ((unsigned char *)data, 
										 XM_OSD_LAYER_FORMAT_Y_UV420,
										 width,
										 height,
										 width,
										 d_x, d_y,
										 d_w, d_h
										);
	if(ret < 0)
	{
		XM_printf ("XM_lcdc_flush_dirty_region failed\n");
	}
}


// 嵌入本地时间戳信息到回放视频中, 仅支持Y_UV420格式
void embed_local_time_into_rendervideo_frame (
													 unsigned int width,			// 视频宽度
													 unsigned int height,		// 视频高度
													 unsigned int stride,	// Y_UV行字节长度
													 char *data					// Y_UV缓冲区
													)
{
	XM_IMAGE *image;
	char str[32];
	int x;			// 相对于视频左上角(原点)的偏移
	int y;
	
	int d_x, d_y;
	int d_w, d_h;
	int ret;
	
	int i, count;
	int space_width = 2;
	int font_width;		// 描边字符的宽度
	DWORD dwTicket;
	DWORD hour, minute, second;
	DWORD tmpyear,tmpmonth,tmpday,tmphour, tmpminute, tmpsecond;
	
	font_width = image_datetime[0]->width;

	XM_lock ();
	dwTicket = XM_GetTickCount() - rendervideo_start_ticket;
	if(dwTicket < 0)
		dwTicket = 0;
	XM_unlock ();
	
	tmpyear = rendervideotime.wYear;
	tmpmonth = rendervideotime.bMonth; 
	tmpday = rendervideotime.bDay;
	tmphour= rendervideotime.bHour;
	tmpminute= rendervideotime.bMinute;
	tmpsecond= rendervideotime.bSecond;

	dwTicket /= 1000;
	second = dwTicket % 60;
	dwTicket /= 60;
	minute = dwTicket % 60;
	hour = dwTicket / 60;
	
	tmpsecond += second;
	dispsecond = (tmpsecond%60);
	
	tmpminute += (tmpsecond/60);
	tmpminute += minute;
	dispminute = tmpminute%60;
	
	tmphour += (tmpminute/60);
	tmphour +=hour;
	disphour = tmphour%24;
	
	if((tmpmonth == 1)||(tmpmonth == 3)||(tmpmonth == 5)||\
		(tmpmonth == 7)||(tmpmonth == 8)||(tmpmonth == 10)||\
		(tmpmonth == 12))
	{
		tmpday += (tmphour/24);
		dispday = tmpday%31;
		tmpmonth +=tmpday/31;
	}
	else if((tmpmonth == 4)||(tmpmonth == 6)||\
			(tmpmonth == 9)||(tmpmonth == 11))
	{
		tmpday += (tmphour/24);
		dispday = tmpday%30;
		tmpmonth +=tmpday/30;
	}
	else if(tmpmonth == 2)
	{
	    if(tmpyear%4)
	    {
		    tmpday += (tmphour/24);
			dispday = tmpday%28;
			tmpmonth +=tmpday/28;
	    }
		else
		{
		    tmpday += (tmphour/24);
			dispday = tmpday%29;
			tmpmonth +=tmpday/29;
		}
	}
	dispmonth = tmpmonth%12;
	tmpyear +=tmpmonth/12;
	dispyear = tmpyear;	
	
	sprintf (str, "%4d/%02d/%02d %02d:%02d:%02d", dispyear,dispmonth,dispday,disphour, dispminute, dispsecond);
	count = strlen (str);
	
	x = 1200;
	y = 10;
	
	d_x = x;
	d_y = y;
	d_w = image_datetime[0]->width * count;
	d_h = image_datetime[0]->height;
	
	XM_lcdc_inv_region ((unsigned char *)data, 
										 XM_OSD_LAYER_FORMAT_Y_UV420,
										 width,
										 height,
										 width,
										 d_x, d_y,
										 d_w, d_h
										);
	
	
	for (i = 0; i < count; i++)
	{
		if (str[i] >= '0' && str[i] <= '9')
		{
			image = image_datetime[str[i] - '0'];	
		}
		else if(str[i] == ':')
			image = image_datetime[10];
		else if(str[i] == '/')
			image = image_datetime[11];
		else
			image = NULL;
		if(image)
		{
			XM_ImageBlend_argb888_to_yuv420_normal (
					image,
					0,											// 复制区域在源图像的偏移(相对源图像平面的左上角)
					0,
					image->width, 							// 待复制区域的像素宽度、像素高度		
					image->height,
					(unsigned char *)data,			// 目标OSD的视频数据缓冲区
					XM_OSD_LAYER_FORMAT_Y_UV420,		// 目标OSD的视频格式
					width, height,							// 目标OSD的像素宽度、像素高度
					stride,								// 目标OSD的行字节长度
					x, y 										//	复制区域在OSD层的偏移 (相对OSD平面的左上角)
					);								
			
			
			x += font_width;
		}
		else
		{
			x += font_width;	//space_width;
		}
	}
		
	ret = XM_lcdc_flush_dirty_region ((unsigned char *)data, 
										 XM_OSD_LAYER_FORMAT_Y_UV420,
										 width,
										 height,
										 width,
										 d_x, d_y,
										 d_w, d_h
										);
	if(ret < 0)
	{
		XM_printf ("XM_lcdc_flush_dirty_region failed\n");
	}
}
	

// 显示已工作时间
static void show_time (xm_osd_framebuffer_t framebuffer, unsigned int day, unsigned int hour, unsigned int minute)
{
	XM_IMAGE *image;
	char str[12];
	int size;
	int i, count;
	unsigned int x, y;

	sprintf (str, "%02d:%02d:%02d", day, hour, minute);
	count = strlen (str);
	size = count * DATETIME_CHAR_RED_WIDTH;

	//x = (icon_scheme.osd_w - size) / 2-160;
	x = 20;
	//y = icon_scheme.osd_h - icon_scheme.vert_space - DATETIME_CHAR_HEIGHT - (icon_scheme.icon_h - DATETIME_CHAR_HEIGHT)/2;
	//y = icon_scheme.vert_space;
	y = 60;
	
	for (i = 0; i < count; i++)
	{
		if (str[i] >= '0' && str[i] <= '9')
		{
			image = image_datetime_red[str[i] - '0'];	
		}
		else if(str[i] == ':')
			image = image_datetime_red[10];
		else
			image = image_datetime_red[11];
		if(image)
		{
			XM_ImageBlendToFrameBuffer (
					image,
					0, 0,
					image->width, image->height,
					framebuffer,
					x,
					y
					);
		}
		x += DATETIME_CHAR_RED_WIDTH;
	}
}

// 显示本地时间
static void show_local_time (xm_osd_framebuffer_t framebuffer)
{
	XM_IMAGE *image;
	XMSYSTEMTIME local_time;
	char str[24];
	int size;
	int i, count;
	int x, y;

	XM_GetLocalTime(&local_time);

	sprintf (str, "%04d/%02d/%02d %02d:%02d:%02d", local_time.wYear,local_time.bMonth, local_time.bDay, local_time.bHour, local_time.bMinute,local_time.bSecond);
	count = strlen (str);
	size = (count - 1) * DATETIME_CHAR_WIDTH + icon_scheme.space_width;

	//x = icon_scheme.osd_w - size - icon_scheme.horz_space;//-200;
	x = 20;
	y = icon_scheme.osd_h - icon_scheme.vert_space - DATETIME_CHAR_HEIGHT - (icon_scheme.icon_h - DATETIME_CHAR_HEIGHT)/2;
	
	for (i = 0; i < count; i++)
	{
		if (str[i] >= '0' && str[i] <= '9')
		{
			image = image_datatime_32[str[i] - '0'];	
		}
		else if(str[i] == ':')
			image = image_datatime_32[10];
		else if(str[i] == '/')
			image = image_datatime_32[11];
		else
			image = NULL;
		if(image)
		{
			XM_ImageBlendToFrameBuffer (
					image,
					0, 0,
					image->width, image->height,
					framebuffer,
					x,
					y
					);
			x += DATETIME_CHAR_WIDTH;
		}
		else
		{
			x += icon_scheme.space_width;
		}

	}
}

static void show_rx_parameter(xm_osd_framebuffer_t framebuffer)
{
	XM_IMAGE *image;
	unsigned char str[128];
	int size;
	int i, count;
	int x, y;

	unsigned char bright = g_rxchp_video_param.brightness;
	unsigned char contrast = g_rxchp_video_param.contrast;
	unsigned char saturation = g_rxchp_video_param.saturation;
	unsigned char hue = g_rxchp_video_param.hue;
	unsigned char sharpness = g_rxchp_video_param.sharpness;
	unsigned char ob = g_rxchp_video_param.ob;
	unsigned char bw = g_rxchp_video_param.bw;

	sprintf(str, "%d/%d/%d/%d/%d/%d/%d", bright, contrast, saturation, hue, sharpness, ob, bw);
	count = strlen (str);
	size = (count - 1) * DATETIME_CHAR_WIDTH + icon_scheme.space_width;

	//x = icon_scheme.osd_w - size - icon_scheme.horz_space;//-200;
	x = 20;
	y = 500;
	
	for (i = 0; i < count; i++)
	{
		if (str[i] >= '0' && str[i] <= '9')
		{
			image = image_datatime_32[str[i] - '0'];	
		}
		else if(str[i] == ':')
			image = image_datatime_32[10];
		else if(str[i] == '/')
			image = image_datatime_32[11];
		else
			image = NULL;
		if(image)
		{
			XM_ImageBlendToFrameBuffer (
					image,
					0, 0,
					image->width, image->height,
					framebuffer,
					x,
					y
					);
			x += DATETIME_CHAR_WIDTH;
		}
		else
		{
			x += icon_scheme.space_width;
		}
	}
}

void Show_recoder_time(xm_osd_framebuffer_t framebuffer)
{
	DWORD dwTicket,hour, minute, second;
	
	//获取当前视频已录制时间
	//嵌入当前录像已录制时间 (分钟:秒)
	if(XM_GetFmlDeviceCap(DEVCAP_VIDEO_REC) == DEVCAP_VIDEO_REC_START)	// 录制状态， 显示当前视频的已录像时间
		dwTicket = XMSYS_H264CodecGetCurrentVideoRecoringTime ();
	else
		dwTicket = 0;		// 非录制状态，显示时间为0

	dwTicket /= 1000;
	second = dwTicket % 60;
	dwTicket /= 60;
	minute = dwTicket % 60;
	hour = dwTicket / 60;

	if(XM_GetFmlDeviceCap (DEVCAP_VIDEO_REC)==DEVCAP_VIDEO_REC_START)  // 录制状态， 显示当前视频的已录像时间
	{
		show_time(framebuffer, hour, minute, second);
	}
}

void show_red_dot(xm_osd_framebuffer_t framebuffer)
{
	unsigned int x, y;
	unsigned int state;
	XM_IMAGE *lpImage;
	
	// 显示录像状态
	x = icon_scheme.horz_space;
	y = icon_scheme.vert_space;

	//录像红点
	state = XM_GetFmlDeviceCap(DEVCAP_VIDEO_REC);
	if(state==DEVCAP_VIDEO_REC_START)//录像中
	{
		if(icon_image[AP_ICON_REC_RED_DOT] == NULL || icon_state[AP_ICON_REC_RED_DOT] == AP_ICON_DISPLAY_OFF)
		{
			icon_state[AP_ICON_REC_RED_DOT] = AP_ICON_DISPLAY_ON;
			load_image(AP_ICON_REC_RED_DOT, AP_ICON_DISPLAY_ON);
		}
		lpImage = icon_image[AP_ICON_REC_RED_DOT];
		if(lpImage)
		{// 录像状态下每隔1秒交替 显示 "录像中" / 隐藏 "录像中"
			int show_icon = 1;
			unsigned int ticket = XM_GetTickCount();
			ticket >>= 10;
			if(ticket & 1)
				show_icon = 0; 
			else
				show_icon = 1;
			
			if(show_icon)
			{
				XM_ImageBlendToFrameBuffer (
						lpImage, 
						0, 0,
						lpImage->width, lpImage->height,
						framebuffer,
						(unsigned int)100,
						(unsigned int)10
						);
			}
		}
	}
}


void show_sd_status(xm_osd_framebuffer_t framebuffer)
{
	unsigned int icon_current_state, state;
	XM_IMAGE *lpImage;

	// 显示SD卡状态
	// 检查卡状态是否变更
	icon_current_state = XM_GetFmlDeviceCap(DEVCAP_SDCARDSTATE);
	{
		if(icon_image[AP_ICON_CARD] == NULL || icon_state[AP_ICON_CARD] != icon_current_state)
		{
			icon_state[AP_ICON_CARD] = icon_current_state;
			load_image(AP_ICON_CARD, icon_current_state);
		}
		lpImage = icon_image[AP_ICON_CARD];
		if(lpImage)
		{
		      
			XM_ImageBlendToFrameBuffer (
						lpImage, 
						0, 0,
						lpImage->width, lpImage->height,
						framebuffer,
						//(unsigned int)(icon_scheme.x_ratio * x + 0.5) + (icon_scheme.icon_w - lpImage->width) / 2-540,
						//(unsigned int)(icon_scheme.y_ratio * y + 0.5) + (icon_scheme.icon_h - lpImage->height) / 2+10
						(unsigned int)(60),
						(unsigned int)(10)
						);
		}
	}
}

void show_mode_icon(xm_osd_framebuffer_t framebuffer)
{
	XM_IMAGE *lpImage;

	//模式图标，录像or拍照
	if(icon_image[AP_ICON_MODE] == NULL || icon_state[AP_ICON_MODE] == AP_ICON_DISPLAY_OFF)
	{
		icon_state[AP_ICON_MODE] = AP_ICON_DISPLAY_ON;
		load_image(AP_ICON_MODE, AP_ICON_DISPLAY_ON);
	}
	lpImage = icon_image[AP_ICON_MODE];
	if(lpImage)
	{
	      
		XM_ImageBlendToFrameBuffer (
					lpImage, 
					0, 0,
					lpImage->width, lpImage->height,
					framebuffer,
					(unsigned int)(20),
					(unsigned int)(10)
					);
	}
}


// ICON显示方案1 (录像状态)
      //     方案1 ICON排列
      //  
      //     录像状态/录像分辨率                                            卡状态 主电池/USB充电标志
      //                   
      //
      // 
      //     MIC/VOICE                     已录时间                         系统时间                
// 使用24X24, 32X32用于ICON的显示

//extern char *Show_Format;
static void display_record_state_icons_scheme_1 (xm_osd_framebuffer_t framebuffer)
{
	unsigned int x, y;
	unsigned int icon_current_state, state;
	XM_IMAGE *lpImage;

	if(framebuffer == NULL)
		return;
	
	// 嵌入标志水印
	#if 0
	//录像视频嵌入时间水印
	//XM_printf(">>>>>>>>AP_GetMenuItem(APPMENUITEM_TIME_STAMP):%d\r\n", AP_GetMenuItem(APPMENUITEM_TIME_STAMP));
	if(packet && (AP_GetMenuItem(APPMENUITEM_TIME_STAMP) == AP_SETTING_VIDEO_TIMESTAMP_ON))
	{
		void embed_local_time_into_video_frame(
												unsigned int width,			// 视频宽度
												unsigned int height,		// 视频高度
												unsigned int stride,		// Y_UV行字节长度
												char *data					// Y_UV缓冲区
												);
				
		unsigned int vaddr = page_addr_to_vaddr((unsigned int)packet->buffer);
		unsigned char *virtual_address = (unsigned char *)vaddr;
		// 嵌入时间戳信息
		// 使用虚地址(Cache)优化算法
		embed_local_time_into_video_frame (packet->width, packet->height, 
															  packet->width, 
															  //packet->buffer
															  (char *)virtual_address
															);		
				
	}
	#endif
	
	#if 0
	XM_printf(">>>>>>>>>AP_GetMenuItem (APPMENUITEM_FLAG_STAMP):%d\r\n", AP_GetMenuItem (APPMENUITEM_FLAG_STAMP));
	if(AP_GetMenuItem (APPMENUITEM_FLAG_STAMP) == AP_SETTING_VIDEO_TIMESTAMP_ON)
	{
		if(AP_GetMenuItem (APPMENUITEM_FLAG_STAMP_UPDATE))
		{
			if(flag_image)
			{
				XM_heap_free (flag_image);
				flag_image = NULL;
			}
		}

		if(flag_image)
		{
			// 将标记水印居中显示
			unsigned int off_x = (icon_scheme.osd_w - flag_image->width) / 2;
			unsigned int off_y = (icon_scheme.osd_h - flag_image->height) / 2;
			XM_ImageBlendToFrameBuffer (
					flag_image, 
					0, 0,
					flag_image->width, flag_image->height,
					framebuffer,
					off_x,
					off_y
					);
		}
	}
	#endif
	
	#if 0
	//开启录像时嵌入已运行时间 小时:分钟
	//if( XM_GetFmlDeviceCap (DEVCAP_VIDEO_REC)==DEVCAP_VIDEO_REC_START)
	{
		dwTicket = XM_GetTickCount () - AppGetStartupTicket ();
		dwTicket /= 1000;
		second = dwTicket % 60;
		dwTicket /= 60;
		minute = dwTicket % 60;
		hour = dwTicket / 60;
		show_time (framebuffer, hour, minute, second);
	}
	#else

	#endif
	
	#if 0	
 	// 显示MIC状态
	// 检查MIC状态是否变更
	x = 1208;
	y = icon_scheme.vert_space+10;
	
	icon_current_state = AP_GetMenuItem (APPMENUITEM_MIC);
	if(icon_image[AP_ICON_MIC] == NULL || icon_state[AP_ICON_MIC] != icon_current_state)
	{
	      
		icon_state[AP_ICON_MIC] = icon_current_state;
		load_image (AP_ICON_MIC, icon_current_state);
	}
	lpImage = icon_image[AP_ICON_MIC];
	if(lpImage)
	{
	     
		XM_ImageBlendToFrameBuffer (
					lpImage, 
					0, 0,
					lpImage->width, lpImage->height,
					framebuffer,
					(unsigned int)(x),
					(unsigned int)(y)+(icon_scheme.icon_h - lpImage->height)/2
				);
	}
	
	// 显示VOICE 状态
	// 检查VOICE 状态是否变更
	x = 1272;
	y = icon_scheme.vert_space+10;
	
	icon_current_state = AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS);
	if(icon_image[AP_ICON_VOICE] == NULL || icon_state[AP_ICON_VOICE] != icon_current_state)
	{
		icon_state[AP_ICON_VOICE] = icon_current_state;
		load_image (AP_ICON_VOICE, icon_current_state);
	}
	lpImage = icon_image[AP_ICON_VOICE];
	if(lpImage)
	{
		XM_ImageBlendToFrameBuffer (
					lpImage, 
					0, 0,
					lpImage->width, lpImage->height,
					framebuffer,
					(unsigned int)(x),
					(unsigned int)(y)+(icon_scheme.icon_h - lpImage->height)/2
					);
	}
     
	// 显示SD卡状态
	// 检查卡状态是否变更
	x = 1336;
	y = icon_scheme.vert_space+10;
	icon_current_state = XM_GetFmlDeviceCap (DEVCAP_SDCARDSTATE);
	{
		if(icon_image[AP_ICON_CARD] == NULL || icon_state[AP_ICON_CARD] != icon_current_state)
		{
			icon_state[AP_ICON_CARD] = icon_current_state;
			load_image (AP_ICON_CARD, icon_current_state);
		}
		lpImage = icon_image[AP_ICON_CARD];
		if(lpImage)
		{
		      
			XM_ImageBlendToFrameBuffer (
						lpImage, 
						0, 0,
						lpImage->width, lpImage->height,
						framebuffer,
						(unsigned int)(x),
						(unsigned int)(y)+(icon_scheme.icon_h - lpImage->height)/2
						);
		}
	}
	
	// 显示电池或充电状态
	// 检查主电池状态是否变更
	x = 1400;
	y = icon_scheme.vert_space+10;
	
	// 显示USB或充电状态
	// 检查USB是否插入充电中
	icon_current_state = xm_power_check_usb_on();//XM_GetFmlDeviceCap (DEVCAP_USB);
	if(icon_image[AP_ICON_USB] == NULL || icon_state[AP_ICON_USB] != icon_current_state)
	{
		icon_state[AP_ICON_USB] = icon_current_state;
		load_image (AP_ICON_USB, icon_current_state);
	}
	if(icon_current_state == DEVCAP_USB_CONNECT_CHARGE)
	{
		lpImage = icon_image[AP_ICON_USB];
		if(lpImage)
		{
			XM_ImageBlendToFrameBuffer (
						lpImage, 
						0, 0,
						lpImage->width, lpImage->height,
						framebuffer,
						(unsigned int)(x),
						(unsigned int)(y)+(icon_scheme.icon_h - lpImage->height)/2
						);
		}
	}
	else
	{
		icon_current_state = XM_GetFmlDeviceCap (DEVCAP_MAINBATTERYVOLTAGE);
		if(icon_image[AP_ICON_MAIN_BATTERY] == NULL || icon_state[AP_ICON_MAIN_BATTERY] != icon_current_state)
		{
			icon_state[AP_ICON_MAIN_BATTERY] = icon_current_state;
			load_image (AP_ICON_MAIN_BATTERY, icon_current_state);
		}
		lpImage = icon_image[AP_ICON_MAIN_BATTERY];
		if(lpImage)
		{
			XM_ImageBlendToFrameBuffer (
						lpImage, 
						0, 0,
						lpImage->width, lpImage->height,
						framebuffer,
						(unsigned int)(x),
						(unsigned int)(y)+(icon_scheme.icon_h - lpImage->height)/2
						);
		}
	}

	if(DispDesktopMenu)
	{
		//显示SYS SETTING
		icon_current_state =AP_GetMenuItem (APPMENUITEM_SYS_SETTING);
		if(icon_image[AP_ICON_SYS_SETTING] == NULL || icon_state[AP_ICON_SYS_SETTING] != icon_current_state)
		{
			icon_state[AP_ICON_SYS_SETTING] = icon_current_state;
			load_image (AP_ICON_SYS_SETTING, icon_current_state);
		}
		lpImage = icon_image[AP_ICON_SYS_SETTING];
		
		x = 150+200*0;
		y = 400 - 100 - icon_scheme.vert_space;
		if(lpImage)
		{
			XM_ImageBlendToFrameBuffer (
						lpImage, 
						0, 0,
						lpImage->width, lpImage->height,
						framebuffer,
						(unsigned int)(x),
						(unsigned int)(y)
						);
		}

		//显示VIDEO SWITCH
		icon_current_state =AP_GetMenuItem (APPMENUITEM_VIDEO_SWITCH);
		if(icon_image[AP_ICON_VIDEO_SWITCH] == NULL || icon_state[AP_ICON_VIDEO_SWITCH] != icon_current_state)
		{
			icon_state[AP_ICON_VIDEO_SWITCH] = icon_current_state;
			load_image (AP_ICON_VIDEO_SWITCH, icon_current_state);
		}
		lpImage = icon_image[AP_ICON_VIDEO_SWITCH];
		
		x = 150+200*1;
		y = 400 - 100 - icon_scheme.vert_space;
		if(lpImage)
		{
			XM_ImageBlendToFrameBuffer (
						lpImage, 
						0, 0,
						lpImage->width, lpImage->height,
						framebuffer,
						(unsigned int)(x),
						(unsigned int)(y)
						);
		}


		//显示PHOTOGRAPH
		icon_current_state =AP_GetMenuItem (APPMENUITEM_PHOTOGRAPH);
		if(icon_image[AP_ICON_PHOTOGRAPH] == NULL || icon_state[AP_ICON_PHOTOGRAPH] != icon_current_state)
		{
			icon_state[AP_ICON_PHOTOGRAPH] = icon_current_state;
			load_image (AP_ICON_PHOTOGRAPH, icon_current_state);
		}
		lpImage = icon_image[AP_ICON_PHOTOGRAPH];
		
		x = 150+200*2;
		y = 400 - 100 - icon_scheme.vert_space;
		if(lpImage)
		{
			XM_ImageBlendToFrameBuffer (
						lpImage, 
						0, 0,
						lpImage->width, lpImage->height,
						framebuffer,
						(unsigned int)(x),
						(unsigned int)(y)
						);
		}

		//显示RECORD SWITCH
		//icon_current_state =AP_GetMenuItem (APPMENUITEM_RECORD_SWITCH);
		icon_current_state =XM_GetFmlDeviceCap(DEVCAP_VIDEO_REC);
		if(icon_image[AP_ICON_RECORD_SWITCH] == NULL || icon_state[AP_ICON_RECORD_SWITCH] != icon_current_state)
		{
			icon_state[AP_ICON_RECORD_SWITCH] = icon_current_state;
			load_image (AP_ICON_RECORD_SWITCH, icon_current_state);
		}
		lpImage = icon_image[AP_ICON_RECORD_SWITCH];
		
		x = 150+200*3;
		y = 400 - 100 - icon_scheme.vert_space;
		if(lpImage)
		{
			XM_ImageBlendToFrameBuffer (
						lpImage, 
						0, 0,
						lpImage->width, lpImage->height,
						framebuffer,
						(unsigned int)(x),
						(unsigned int)(y)
						);
		}

		//显示RECORD LOCK
		icon_current_state = AP_GetMenuItem (APPMENUITEM_LOCK_MODE);
		if(icon_image[AP_ICON_RECORD_LOCK] == NULL || icon_state[AP_ICON_RECORD_LOCK] != icon_current_state)
		{
			icon_state[AP_ICON_RECORD_LOCK] = icon_current_state;
			load_image (AP_ICON_RECORD_LOCK, icon_current_state);
		}
		lpImage = icon_image[AP_ICON_RECORD_LOCK];
		
		x = 150+200*4;
		y = 400 - 100 - icon_scheme.vert_space;
		if(lpImage)
		{
			XM_ImageBlendToFrameBuffer (
						lpImage, 
						0, 0,
						lpImage->width, lpImage->height,
						framebuffer,
						(unsigned int)(x),
						(unsigned int)(y)
						);
		}

		
		//显示RECORD LIST
		icon_current_state =AP_GetMenuItem (APPMENUITEM_RECORD_LIST);
		if(icon_image[AP_ICON_RECORD_LIST] == NULL || icon_state[AP_ICON_RECORD_LIST] != icon_current_state)
		{
			icon_state[AP_ICON_RECORD_LIST] = icon_current_state;
			load_image (AP_ICON_RECORD_LIST, icon_current_state);
		}
		lpImage = icon_image[AP_ICON_RECORD_LIST];
		
		x = 150+200*5;
		y = 400 - 100 - icon_scheme.vert_space;
		if(lpImage)
		{
			XM_ImageBlendToFrameBuffer (
						lpImage, 
						0, 0,
						lpImage->width, lpImage->height,
						framebuffer,
						(unsigned int)(x),
						(unsigned int)(y)
						);
		}

		//显示MIC SWITCH
		icon_current_state =AP_GetMenuItem (APPMENUITEM_MIC_SWITCH);
		if(icon_image[AP_ICON_MIC_SWITCH] == NULL || icon_state[AP_ICON_MIC_SWITCH] != icon_current_state)
		{
			icon_state[AP_ICON_MIC_SWITCH] = icon_current_state;
			load_image (AP_ICON_MIC_SWITCH, icon_current_state);
		}
		lpImage = icon_image[AP_ICON_MIC_SWITCH];
		
		x = 150+200*6;
		y = 400 - 100 - icon_scheme.vert_space;
		if(lpImage)
		{
			XM_ImageBlendToFrameBuffer (
						lpImage, 
						0, 0,
						lpImage->width, lpImage->height,
						framebuffer,
						(unsigned int)(x),
						(unsigned int)(y)
						);
		}
	}
	#endif
	#if 0
	x = icon_scheme.horz_space;
	y = 240 - icon_scheme.vert_space;
	
	// 检查day night状态是否变更
	icon_current_state = AP_GetMenuItem (APPMENUITEM_DAY_NIGHT_MODE);

	if(icon_image[AP_ICON_DAY_NIGHT_MODE] == NULL || icon_state[AP_ICON_DAY_NIGHT_MODE] != icon_current_state)
	{
	       
		icon_state[AP_ICON_DAY_NIGHT_MODE] = icon_current_state;
		load_image (AP_ICON_DAY_NIGHT_MODE, icon_current_state);
	
	}
	lpImage = icon_image[AP_ICON_DAY_NIGHT_MODE];
	if(lpImage)
	{
	     
		XM_ImageBlendToFrameBuffer (
					lpImage, 
					0, 0,
					lpImage->width, lpImage->height,
					framebuffer,
					(unsigned int)(783),
					(unsigned int)(20) 
				);
	}
	
    // 检查LOCK状态是否变更
	icon_current_state = AP_GetMenuItem (APPMENUITEM_LOCK_MODE);
	 // XM_printf("icon_current_state is %d\n",icon_current_state);
	if(icon_image[AP_ICON_LOCK] == NULL || icon_state[AP_ICON_LOCK] != icon_current_state)
	{
	       
		icon_state[AP_ICON_LOCK] = icon_current_state;
		load_image (AP_ICON_LOCK, icon_current_state);
	
	}
	
	lpImage = icon_image[AP_ICON_LOCK];
	if(lpImage)
	{
	     
		XM_ImageBlendToFrameBuffer (
					lpImage, 
					0, 0,
					lpImage->width, lpImage->height,
					framebuffer,
					(unsigned int)(787),
					(unsigned int)(170) 
				);
	}

	// 检查MIC状态是否变更
	icon_current_state = AP_GetMenuItem (APPMENUITEM_MIC);
	if(icon_image[AP_ICON_MIC] == NULL || icon_state[AP_ICON_MIC] != icon_current_state)
	{
	      
		icon_state[AP_ICON_MIC] = icon_current_state;
		load_image (AP_ICON_MIC, icon_current_state);
	}
	lpImage = icon_image[AP_ICON_MIC];
	if(lpImage)
	{
	     
		XM_ImageBlendToFrameBuffer (
					lpImage, 
					0, 0,
					lpImage->width, lpImage->height,
					framebuffer,
					(unsigned int)(783),
					(unsigned int)(330) 
				);
	}
    
	// 显示PHOTO
	icon_current_state = AP_GetMenuItem (APPMENUITEM_PHOTO);
	if(icon_image[AP_ICON_PHOTO] == NULL || icon_state[AP_ICON_PHOTO] != icon_current_state)
	{
		icon_state[AP_ICON_PHOTO] = icon_current_state;
		load_image (AP_ICON_PHOTO, icon_current_state);
	}
	lpImage = icon_image[AP_ICON_PHOTO];
	if(lpImage)
	{
		XM_ImageBlendToFrameBuffer (
					lpImage, 
					0, 0,
					lpImage->width, lpImage->height,
					framebuffer,
					(unsigned int)(780),
					(unsigned int)(250)
					);
	}
	
	// 显示HOME
	icon_current_state =1;// AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS);
	if(icon_image[AP_ICON_HOME] == NULL || icon_state[AP_ICON_HOME] != icon_current_state)
	{
		icon_state[AP_ICON_HOME] = icon_current_state;
		load_image (AP_ICON_HOME, icon_current_state);
	}
	lpImage = icon_image[AP_ICON_HOME];
	if(lpImage)
	{
		XM_ImageBlendToFrameBuffer (
					lpImage, 
					0, 0,
					lpImage->width, lpImage->height,
					framebuffer,
					(unsigned int)(783),
					(unsigned int)(400)
					);
	}
	  
	x = 320 - icon_scheme.horz_space - icon_scheme.icon_w;
	y = icon_scheme.vert_space;
	
	// 显示USB或充电状态
	// 检查USB是否插入充电中
	icon_current_state = DEVCAP_USB_CONNECT_CHARGE;  //XM_GetFmlDeviceCap (DEVCAP_USB);
	if(icon_image[AP_ICON_USB] == NULL || icon_state[AP_ICON_USB] != icon_current_state)
	{
		icon_state[AP_ICON_USB] = icon_current_state;
		load_image (AP_ICON_USB, icon_current_state);
	}
	if(icon_current_state != DEVCAP_USB_DISCONNECT)
	{
		lpImage = icon_image[AP_ICON_USB];
		if(lpImage)
		{
			XM_ImageBlendToFrameBuffer (
						lpImage, 
						0, 0,
						lpImage->width, lpImage->height,
						framebuffer,
						(unsigned int)(icon_scheme.x_ratio * x + 0.5) + (icon_scheme.icon_w - lpImage->width) / 2,
						(unsigned int)(icon_scheme.y_ratio * y + 0.5) + (icon_scheme.icon_h - lpImage->height) / 2
						);
		}
	
		x -= icon_scheme.icon_space + icon_scheme.icon_w;
	}
	
	 // 显示VOICE 状态
	//x += icon_scheme.icon_space + icon_scheme.icon_w;
	// 检查VOICE 状态是否变更
	icon_current_state = AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS);
	if(icon_image[AP_ICON_VOICE] == NULL || icon_state[AP_ICON_VOICE] != icon_current_state)
	{
		icon_state[AP_ICON_VOICE] = icon_current_state;
		load_image (AP_ICON_VOICE, icon_current_state);
	}
	lpImage = icon_image[AP_ICON_VOICE];
	if(lpImage)
	{
		XM_ImageBlendToFrameBuffer (
					lpImage, 
					0, 0,
					lpImage->width, lpImage->height,
					framebuffer,
					(unsigned int)(icon_scheme.x_ratio * x + 0.5) + (icon_scheme.icon_w - lpImage->width) / 2-600,
					(unsigned int)(icon_scheme.y_ratio * y + 0.5) + (icon_scheme.icon_h - lpImage->height) / 2+10
					);
	}
     
	// 显示SD卡状态
	// 检查卡状态是否变更
	icon_current_state = XM_GetFmlDeviceCap (DEVCAP_SDCARDSTATE);

	{
		if(icon_image[AP_ICON_CARD] == NULL || icon_state[AP_ICON_CARD] != icon_current_state)
		{
			icon_state[AP_ICON_CARD] = icon_current_state;
			load_image (AP_ICON_CARD, icon_current_state);
		}
		lpImage = icon_image[AP_ICON_CARD];
		if(lpImage)
		{
		      
			XM_ImageBlendToFrameBuffer (
						lpImage, 
						0, 0,
						lpImage->width, lpImage->height,
						framebuffer,
						(unsigned int)(icon_scheme.x_ratio * x + 0.5) + (icon_scheme.icon_w - lpImage->width) / 2-540,
						(unsigned int)(icon_scheme.y_ratio * y + 0.5) + (icon_scheme.icon_h - lpImage->height) / 2+10
						);
		}
	}
	
    // 检查day night状态是否变更
	icon_current_state = AP_GetMenuItem (APPMENUITEM_DAY_NIGHT_MODE);
	if(icon_image[AP_ICON_DAY_NIGHT_MODE] == NULL || icon_state[AP_ICON_DAY_NIGHT_MODE] != icon_current_state)
	{
	       
		icon_state[AP_ICON_DAY_NIGHT_MODE] = icon_current_state;
		load_image (AP_ICON_DAY_NIGHT_MODE, icon_current_state);
	
	}
	lpImage = icon_image[AP_ICON_DAY_NIGHT_MODE];
	if(lpImage)
	{
	     
		XM_ImageBlendToFrameBuffer (
					lpImage, 
					0, 0,
					lpImage->width, lpImage->height,
					framebuffer,
					(unsigned int)(280),
					(unsigned int)(10) 
				);
	}

	// 显示电池或充电状态
	// 检查主电池状态是否变更
	x -= icon_scheme.icon_space + icon_scheme.icon_w;
	 // 显示USB或充电状态
	// 检查USB是否插入充电中
	icon_current_state = XM_GetFmlDeviceCap (DEVCAP_USB);
	if(icon_image[AP_ICON_USB] == NULL || icon_state[AP_ICON_USB] != icon_current_state)
	{
		icon_state[AP_ICON_USB] = icon_current_state;
		load_image (AP_ICON_USB, icon_current_state);
	}
	if(icon_current_state == DEVCAP_USB_CONNECT_CHARGE)
	{
		lpImage = icon_image[AP_ICON_USB];
		if(lpImage)
		{
			XM_ImageBlendToFrameBuffer (
						lpImage, 
						0, 0,
						lpImage->width, lpImage->height,
						framebuffer,
						(unsigned int)(icon_scheme.x_ratio * x + 0.5) + (icon_scheme.icon_w - lpImage->width) / 2-580,
						(unsigned int)(icon_scheme.y_ratio * y + 0.5) + (icon_scheme.icon_h - lpImage->height) / 2+10

						);
		}
	
		x -= icon_scheme.icon_space + icon_scheme.icon_w;
	}
	else
	{
		icon_current_state = XM_GetFmlDeviceCap (DEVCAP_MAINBATTERYVOLTAGE);
		if(icon_image[AP_ICON_MAIN_BATTERY] == NULL || icon_state[AP_ICON_MAIN_BATTERY] != icon_current_state)
		{
			icon_state[AP_ICON_MAIN_BATTERY] = icon_current_state;
			load_image (AP_ICON_MAIN_BATTERY, icon_current_state);
		}
		lpImage = icon_image[AP_ICON_MAIN_BATTERY];
		if(lpImage)
		{
			XM_ImageBlendToFrameBuffer (
						lpImage, 
						0, 0,
						lpImage->width, lpImage->height,
						framebuffer,
						(unsigned int)(icon_scheme.x_ratio * x + 0.5) + (icon_scheme.icon_w - lpImage->width) / 2-580,
						(unsigned int)(icon_scheme.y_ratio * y + 0.5) + (icon_scheme.icon_h - lpImage->height) / 2+10
						);
		}
	}
	
	// 显示GPS状态
	x -= icon_scheme.icon_space + icon_scheme.icon_w;
	// 检查GPS状态是否变更
	icon_current_state = XM_GetFmlDeviceCap (DEVCAP_GPSBD);
	if(icon_image[AP_ICON_GPS] == NULL || icon_state[AP_ICON_GPS] != icon_current_state)
	{
		icon_state[AP_ICON_GPS] = icon_current_state;
		load_image (AP_ICON_GPS, icon_current_state);
	}
	lpImage = icon_image[AP_ICON_GPS];
	if(lpImage)
	{
		XM_ImageBlendToFrameBuffer (
					lpImage, 
					0, 0,
					lpImage->width, lpImage->height,
					framebuffer,
					(unsigned int)(icon_scheme.x_ratio * x + 0.5-500),
					(unsigned int)(icon_scheme.y_ratio * y + 0.5)
					);
	}
	#endif
}


void exitdesktopmenu(void)
{
    if(icon_image[AP_ICON_SYS_SETTING])
	{
		XM_ImageDelete (icon_image[AP_ICON_SYS_SETTING]);
		icon_image[AP_ICON_SYS_SETTING] = NULL;
	}

	if(icon_image[AP_ICON_VIDEO_SWITCH])
	{
		XM_ImageDelete (icon_image[AP_ICON_VIDEO_SWITCH]);
		icon_image[AP_ICON_VIDEO_SWITCH] = NULL;
	}

	if(icon_image[AP_ICON_PHOTOGRAPH])
	{
		XM_ImageDelete (icon_image[AP_ICON_PHOTOGRAPH]);
		icon_image[AP_ICON_PHOTOGRAPH] = NULL;
	}

	if(icon_image[AP_ICON_RECORD_SWITCH])
	{
		XM_ImageDelete (icon_image[AP_ICON_RECORD_SWITCH]);
		icon_image[AP_ICON_RECORD_SWITCH] = NULL;
	}

	if(icon_image[AP_ICON_RECORD_LOCK])
	{
		XM_ImageDelete (icon_image[AP_ICON_RECORD_LOCK]);
		icon_image[AP_ICON_RECORD_LOCK] = NULL;
	}

	if(icon_image[AP_ICON_RECORD_LIST])
	{
		XM_ImageDelete (icon_image[AP_ICON_RECORD_LIST]);
		icon_image[AP_ICON_RECORD_LIST] = NULL;
	}
}

// ICON显示方案1 (录像状态)
      //     方案1 ICON排列
      //  
      //     录像状态/录像分辨率                                            卡状态 主电池/USB充电标志
      //                   
      //
      // 
      //     MIC/VOICE                     已录时间                         系统时间                
// 使用24X24, 32X32用于ICON的显示
static void display_photo_state_icons_scheme_1 (xm_osd_framebuffer_t framebuffer)
{
	unsigned int x, y;
	unsigned int icon_current_state;
	XM_IMAGE *lpImage;
	DWORD dwTicket;
	DWORD hour, minute, second;

	if(framebuffer == NULL)
		return;
#if 0
	// 嵌入标志水印
	if(AP_GetMenuItem (APPMENUITEM_FLAG_STAMP) == AP_SETTING_VIDEO_TIMESTAMP_ON)
	{
		if(AP_GetMenuItem (APPMENUITEM_FLAG_STAMP_UPDATE))
		{
			if(flag_image)
			{
				XM_heap_free (flag_image);
				flag_image = NULL;
			}
		}

		if(flag_image)
		{
			// 将标记水印居中显示
			unsigned int off_x = (icon_scheme.osd_w - flag_image->width) / 2;
			unsigned int off_y = (icon_scheme.osd_h - flag_image->height) / 2;
			XM_ImageBlendToFrameBuffer (
					flag_image, 
					0, 0,
					flag_image->width, flag_image->height,
					framebuffer,
					off_x,
					off_y
					);
		}
	}

	// 嵌入已运行时间 小时:分钟
	dwTicket = XM_GetTickCount () - AppGetStartupTicket ();
	dwTicket /= 1000;
	second = dwTicket % 60;
	dwTicket /= 60;
	minute = dwTicket % 60;
	hour = dwTicket / 60;
	show_time (framebuffer, hour, minute, second);

	// 嵌入当前时间 小时:分钟
	show_local_time (framebuffer);

	// 显示录像状态
	x = icon_scheme.horz_space;
	y = icon_scheme.vert_space;
	// 检查录像状态是否变更
	icon_current_state = XM_GetFmlDeviceCap (DEVCAP_VIDEO_REC);
	if(icon_image[AP_ICON_REC] == NULL || icon_state[AP_ICON_REC] != icon_current_state)
	{
		icon_state[AP_ICON_REC] = icon_current_state;
		load_image (AP_ICON_REC, icon_current_state);
	}
	lpImage = icon_image[AP_ICON_REC];
	if(lpImage)
	{
		int show_icon = 1;
		if(icon_state[AP_ICON_REC] == DEVCAP_VIDEO_REC_START)
		{
			// 录像状态下每隔1秒交替 显示 "录像中" / 隐藏 "录像中"
			unsigned int ticket = XM_GetTickCount();
			ticket >>= 10;
			if(ticket & 1)
				show_icon = 0; 
			else
				show_icon = 1;
		}
		else
		{
			show_icon = 1; 
		}
		if(show_icon)
			XM_ImageBlendToFrameBuffer (
					lpImage, 
					0, 0,
					lpImage->width, lpImage->height,
					framebuffer,
					(unsigned int)(icon_scheme.x_ratio * x + 0.5),
					(unsigned int)(icon_scheme.y_ratio * y + 0.5)
					);
	}
	
#endif

    #if 1
	
	
	x = icon_scheme.horz_space;
	y = 240 - icon_scheme.vert_space;
	#if 0
	      // 检查day night状态是否变更
	icon_current_state = AP_GetMenuItem (APPMENUITEM_DAY_NIGHT_MODE);

	if(icon_image[AP_ICON_DAY_NIGHT_MODE] == NULL || icon_state[AP_ICON_DAY_NIGHT_MODE] != icon_current_state)
	{
	       
		icon_state[AP_ICON_DAY_NIGHT_MODE] = icon_current_state;
		load_image (AP_ICON_DAY_NIGHT_MODE, icon_current_state);
	
	}
	lpImage = icon_image[AP_ICON_DAY_NIGHT_MODE];
	if(lpImage)
	{
	     
		XM_ImageBlendToFrameBuffer (
					lpImage, 
					0, 0,
					lpImage->width, lpImage->height,
					framebuffer,
					(unsigned int)(783),
					(unsigned int)(20) 
				);
	}
      // 检查LOCK状态是否变更
	icon_current_state = AP_GetMenuItem (APPMENUITEM_LOCK_MODE);
	 // XM_printf("icon_current_state is %d\n",icon_current_state);
	if(icon_image[AP_ICON_LOCK] == NULL || icon_state[AP_ICON_LOCK] != icon_current_state)
	{
	       
		icon_state[AP_ICON_LOCK] = icon_current_state;
		load_image (AP_ICON_LOCK, icon_current_state);
	
	}
	lpImage = icon_image[AP_ICON_LOCK];
	if(lpImage)
	{
	     
		XM_ImageBlendToFrameBuffer (
					lpImage, 
					0, 0,
					lpImage->width, lpImage->height,
					framebuffer,
					(unsigned int)(787),
					(unsigned int)(110) 
				);
	}
	// 检查MIC状态是否变更
	icon_current_state = AP_GetMenuItem (APPMENUITEM_MIC);
	if(icon_image[AP_ICON_MIC] == NULL || icon_state[AP_ICON_MIC] != icon_current_state)
	{
	      
		icon_state[AP_ICON_MIC] = icon_current_state;
		load_image (AP_ICON_MIC, icon_current_state);
	}
	lpImage = icon_image[AP_ICON_MIC];
	if(lpImage)
	{
	     
		XM_ImageBlendToFrameBuffer (
					lpImage, 
					0, 0,
					lpImage->width, lpImage->height,
					framebuffer,
					(unsigned int)(783),
					(unsigned int)(300) 
				);
	}
       #endif
		
	// 显示PHOTO
	icon_current_state =1;// AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS);
	if(icon_image[AP_ICON_PHOTO] == NULL || icon_state[AP_ICON_PHOTO] != icon_current_state)
	{
		icon_state[AP_ICON_PHOTO] = icon_current_state;
		load_image (AP_ICON_PHOTO, icon_current_state);
	}
	lpImage = icon_image[AP_ICON_PHOTO];
	if(lpImage)
	{
		XM_ImageBlendToFrameBuffer (
					lpImage, 
					0, 0,
					lpImage->width, lpImage->height,
					framebuffer,
					(unsigned int)(780),
					(unsigned int)(200)
					);
	}
		// 显示HOME
	

	icon_current_state =1;// AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS);
	if(icon_image[AP_ICON_HOME] == NULL || icon_state[AP_ICON_HOME] != icon_current_state)
	{
		icon_state[AP_ICON_HOME] = icon_current_state;
		load_image (AP_ICON_HOME, icon_current_state);
	}
	lpImage = icon_image[AP_ICON_HOME];
	if(lpImage)
	{
		XM_ImageBlendToFrameBuffer (
					lpImage, 
					0, 0,
					lpImage->width, lpImage->height,
					framebuffer,
					(unsigned int)(783),
					(unsigned int)(400)
					);
	}
      #endif
	  #if 0
	x = 320 - icon_scheme.horz_space - icon_scheme.icon_w;
	y = icon_scheme.vert_space;
	
	// 显示USB或充电状态
	// 检查USB是否插入充电中
	icon_current_state = XM_GetFmlDeviceCap (DEVCAP_USB);
	if(icon_image[AP_ICON_USB] == NULL || icon_state[AP_ICON_USB] != icon_current_state)
	{
		icon_state[AP_ICON_USB] = icon_current_state;
		load_image (AP_ICON_USB, icon_current_state);
	}
	if(icon_current_state != DEVCAP_USB_DISCONNECT)
	{
		lpImage = icon_image[AP_ICON_USB];
		if(lpImage)
		{
			XM_ImageBlendToFrameBuffer (
						lpImage, 
						0, 0,
						lpImage->width, lpImage->height,
						framebuffer,
						(unsigned int)(icon_scheme.x_ratio * x + 0.5) + (icon_scheme.icon_w - lpImage->width) / 2,
						(unsigned int)(icon_scheme.y_ratio * y + 0.5) + (icon_scheme.icon_h - lpImage->height) / 2
						);
		}
	
		x -= icon_scheme.icon_space + icon_scheme.icon_w;
	}
	
	   // 显示VOICE 状态
	//x += icon_scheme.icon_space + icon_scheme.icon_w;
	// 检查VOICE 状态是否变更
	icon_current_state = AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS);
	if(icon_image[AP_ICON_VOICE] == NULL || icon_state[AP_ICON_VOICE] != icon_current_state)
	{
		icon_state[AP_ICON_VOICE] = icon_current_state;
		load_image (AP_ICON_VOICE, icon_current_state);
	}
	lpImage = icon_image[AP_ICON_VOICE];
	if(lpImage)
	{
		XM_ImageBlendToFrameBuffer (
					lpImage, 
					0, 0,
					lpImage->width, lpImage->height,
					framebuffer,
					(unsigned int)(icon_scheme.x_ratio * x + 0.5) + (icon_scheme.icon_w - lpImage->width) / 2-600,
					(unsigned int)(icon_scheme.y_ratio * y + 0.5) + (icon_scheme.icon_h - lpImage->height) / 2
					);
	}
       #if 1
	// 显示SD卡状态
	
	// 检查卡状态是否变更
	icon_current_state = XM_GetFmlDeviceCap (DEVCAP_SDCARDSTATE);

	{
		if(icon_image[AP_ICON_CARD] == NULL || icon_state[AP_ICON_CARD] != icon_current_state)
		{
			icon_state[AP_ICON_CARD] = icon_current_state;
			load_image (AP_ICON_CARD, icon_current_state);
		}
		lpImage = icon_image[AP_ICON_CARD];
		if(lpImage)
		{
			XM_ImageBlendToFrameBuffer (
						lpImage, 
						0, 0,
						lpImage->width, lpImage->height,
						framebuffer,
						(unsigned int)(icon_scheme.x_ratio * x + 0.5) + (icon_scheme.icon_w - lpImage->width) / 2-540,
						(unsigned int)(icon_scheme.y_ratio * y + 0.5) + (icon_scheme.icon_h - lpImage->height) / 2
						);
		}
	}
       #endif
	// 显示电池或充电状态
	// 检查主电池状态是否变更
	x -= icon_scheme.icon_space + icon_scheme.icon_w;
	icon_current_state = XM_GetFmlDeviceCap (DEVCAP_MAINBATTERYVOLTAGE);
	if(icon_image[AP_ICON_MAIN_BATTERY] == NULL || icon_state[AP_ICON_MAIN_BATTERY] != icon_current_state)
	{
		icon_state[AP_ICON_MAIN_BATTERY] = icon_current_state;
		load_image (AP_ICON_MAIN_BATTERY, icon_current_state);
	}
	lpImage = icon_image[AP_ICON_MAIN_BATTERY];
	if(lpImage)
	{
		XM_ImageBlendToFrameBuffer (
					lpImage, 
					0, 0,
					lpImage->width, lpImage->height,
					framebuffer,
					(unsigned int)(icon_scheme.x_ratio * x + 0.5) + (icon_scheme.icon_w - lpImage->width) / 2-580,
					(unsigned int)(icon_scheme.y_ratio * y + 0.5) + (icon_scheme.icon_h - lpImage->height) / 2
					);
	}
	#endif
	



	
}
extern int Red_Location;
int Camera_Data_signed_Pre = 0;
//extern BOOL NO_Signed_Fail;


/**
 *
 * 视频叠加拍照图标
 *
 */
void video_takephoto_display(xm_osd_framebuffer_t framebuffer)
{
	unsigned int x0, x1, y0, y1;
	XM_IMAGE *lpImage;
	unsigned char curch;

    if((icon_image[AP_ICON_TAKEPHOTO] == NULL) || (icon_state[AP_ICON_TAKEPHOTO] == AP_ICON_DISPLAY_OFF) /*|| (Pre_Blue_Data != AP_GetBlue())||(Camera_Data_signed_Pre != Camera_Data_signed)*/)
	{
		icon_state[AP_ICON_TAKEPHOTO] = 1;
		load_image(AP_ICON_TAKEPHOTO, AP_ICON_DISPLAY_ON);
	}
	
	curch = AP_GetMenuItem(APPMENUITEM_CH);
	switch(curch)
	{
		case CH_AHD1:
			x0 = 512-48;
			y0 = 300-48;
			x1 = 0;
			y1 = 0;
			break;

		case CH_AHD2:
			x0 = 512-48;
			y0 = 300-48;
			x1 = 0;
			y1 = 0;
			break;

		case CH_V_AHD12:
			x0 = 256-48;
			y0 = 300-48;
			x1 = 512+256-48;
			y1 = 300-48;
			break;

		case CH_V_AHD21:
			x0 = 256-48;
			y0 = 300-48;
			x1 = 512+256-48;
			y1 = 300-48;
			break;

		case CH_H_AHD12:
			x0 = 512-48;
			y0 = 150-48;
			x1 = 512-48;
			y1 = 300+150-48;
			break;

		case CH_H_AHD21:
			x0 = 512-48;
			y0 = 150-48;
			x1 = 512-48;
			y1 = 300+150-48;
			break;

		default:
			break;
	}
	
	if(curch<=CH_AHD2)
	{
		lpImage = icon_image[AP_ICON_TAKEPHOTO];
		if(lpImage)
		{
			XM_ImageBlendToFrameBuffer (
						lpImage, 
						0, 0,
						lpImage->width, lpImage->height,
						framebuffer,
						x0,
						y0
						);
		}	
	}
	else
	{
		lpImage = icon_image[AP_ICON_TAKEPHOTO];
		if(lpImage)
		{
			XM_ImageBlendToFrameBuffer (
						lpImage, 
						0, 0,
						lpImage->width, lpImage->height,
						framebuffer,
						x0,
						y0
						);

			XM_ImageBlendToFrameBuffer (
						lpImage, 
						0, 0,
						lpImage->width, lpImage->height,
						framebuffer,
						x1,
						y1
						);
		}	
	}
}

/**
 *
 * 视频叠加通道标识信息
 *
 */
void video_ch_display(xm_osd_framebuffer_t framebuffer)
{
	unsigned int x0, x1, y0, y1;
	XM_IMAGE *lpImage;
	unsigned char curch;

    if((icon_image[AP_ICON_CH_AHD1] == NULL) || (icon_state[AP_ICON_CH_AHD1] == AP_ICON_DISPLAY_OFF) /*|| (Pre_Blue_Data != AP_GetBlue())||(Camera_Data_signed_Pre != Camera_Data_signed)*/)
	{
		icon_state[AP_ICON_CH_AHD1] = 1;
		load_image(AP_ICON_CH_AHD1, AP_ICON_DISPLAY_ON);
	}
    if((icon_image[AP_ICON_CH_AHD2] == NULL) || (icon_state[AP_ICON_CH_AHD2] == AP_ICON_DISPLAY_OFF) /*|| (Pre_Blue_Data != AP_GetBlue())||(Camera_Data_signed_Pre != Camera_Data_signed)*/)
	{
		icon_state[AP_ICON_CH_AHD2] = 1;
		load_image(AP_ICON_CH_AHD2, AP_ICON_DISPLAY_ON);
	}	
	
	curch = AP_GetMenuItem(APPMENUITEM_CH);
	switch(curch)
	{
		case CH_AHD1:
			x0 = 920;
			y0 = 10;
			x1 = 0;
			y1 = 0;
			break;

		case CH_AHD2:
			x0 = 920;
			y0 = 10;
			x1 = 0;
			y1 = 0;
			break;

		case CH_V_AHD12:
			x0 = 420;
			y0 = 10;
			x1 = 920;
			y1 = 10;
			break;

		case CH_V_AHD21:
			x0 = 920;
			y0 = 10;
			x1 = 420;
			y1 = 10;
			break;

		case CH_H_AHD12:
			x0 = 920;
			y0 = 10;
			x1 = 920;
			y1 = 310;
			break;

		case CH_H_AHD21:
			x0 = 920;
			y0 = 310;
			x1 = 920;
			y1 = 10;
			break;

		default:
			break;
	}
	
	if(curch<=CH_AHD2)
	{
		if(curch==CH_AHD1)
		{
			lpImage = icon_image[AP_ICON_CH_AHD1];
		}
		else
		{
			lpImage = icon_image[AP_ICON_CH_AHD2];
		}
		if(lpImage)
		{
			XM_ImageBlendToFrameBuffer (
						lpImage, 
						0, 0,
						lpImage->width, lpImage->height,
						framebuffer,
						x0,
						y0
						);
		}	
	}
	else
	{
		lpImage = icon_image[AP_ICON_CH_AHD1];
		if(lpImage)
		{
			XM_ImageBlendToFrameBuffer (
						lpImage, 
						0, 0,
						lpImage->width, lpImage->height,
						framebuffer,
						x0,
						y0
						);
		}	
		lpImage = icon_image[AP_ICON_CH_AHD2];
		if(lpImage)
		{
			XM_ImageBlendToFrameBuffer (
						lpImage, 
						0, 0,
						lpImage->width, lpImage->height,
						framebuffer,
						x1,
						y1
						);
		}	
	}
}



/**
 *
 * 视频叠加分割线
 *
 */
void video_splitline_display(xm_osd_framebuffer_t framebuffer)
{
	XM_IMAGE *lpImage;
	unsigned char curch;
	
	curch = AP_GetMenuItem(APPMENUITEM_CH);
	switch(curch)
	{
		case CH_AHD1:

			break;

		case CH_AHD2:

			break;

		case CH_V_AHD12:
		case CH_V_AHD21:
	        if(icon_image[AP_ICON_V_LINE] == NULL || icon_state[AP_ICON_V_LINE] == AP_ICON_DISPLAY_OFF)
	    	{
	    		icon_state[AP_ICON_V_LINE] = AP_ICON_DISPLAY_ON;
	    		load_image(AP_ICON_V_LINE, AP_ICON_DISPLAY_ON);
	    	}
	    	lpImage = icon_image[AP_ICON_V_LINE];
	    	if(lpImage)
	    	{
	    		XM_ImageBlendToFrameBuffer (
	    					lpImage, 
	    					0, 0,
	    					lpImage->width, lpImage->height,
	    					framebuffer,
	    					(unsigned int)(512),
	    					(unsigned int)(0)
	    					);
	    	}
			break;

		case CH_H_AHD12:
		case CH_H_AHD21:
	        if(icon_image[AP_ICON_H_LINE] == NULL || icon_state[AP_ICON_H_LINE] == AP_ICON_DISPLAY_OFF)
	    	{
	    		icon_state[AP_ICON_H_LINE] = AP_ICON_DISPLAY_ON;
	    		load_image(AP_ICON_H_LINE, AP_ICON_DISPLAY_ON);
	    	}
	    	lpImage = icon_image[AP_ICON_H_LINE];
	    	if(lpImage)
	    	{
	    		XM_ImageBlendToFrameBuffer (
	    					lpImage, 
	    					0, 0,
	    					lpImage->width, lpImage->height,
	    					framebuffer,
	    					(unsigned int)(0),
	    					(unsigned int)(300)
	    					);
	    	}
			break;
			
		default:
			break;
	}
}


/**
 *
 * 视频叠加无信号标志
 *
 */
void video_nosignal_display(xm_osd_framebuffer_t framebuffer)
{
	XM_IMAGE *lpImage;
	unsigned char curch;
	
	curch = AP_GetMenuItem(APPMENUITEM_CH);
	switch(curch)
	{
		case CH_AHD1:
			if(rxchip_get_ch1_signal_status()==1)//无信号
			{
		        if((icon_image[AP_ICON_NO_SIGNED] == NULL) || (icon_state[AP_ICON_NO_SIGNED] == AP_ICON_DISPLAY_OFF))
		    	{
		    		icon_state[AP_ICON_NO_SIGNED] = AP_ICON_DISPLAY_ON;
		    		load_image(AP_ICON_NO_SIGNED, AP_ICON_DISPLAY_ON);
		    	}
		    	lpImage = icon_image[AP_ICON_NO_SIGNED];
		    	if(lpImage)
		    	{
		    		XM_ImageBlendToFrameBuffer (
		    					lpImage, 
		    					0, 0,
		    					lpImage->width, lpImage->height,
		    					framebuffer,
		    					(unsigned int)(480),
		    					(unsigned int)(280)
		    					);
		    	}
			}
			break;

		case CH_AHD2:
			if(rxchip_get_ch2_signal_status()==1)//无信号
			{
		        if((icon_image[AP_ICON_NO_SIGNED] == NULL) || (icon_state[AP_ICON_NO_SIGNED] == AP_ICON_DISPLAY_OFF))
		    	{
		    		icon_state[AP_ICON_NO_SIGNED] = AP_ICON_DISPLAY_ON;
		    		load_image(AP_ICON_NO_SIGNED, AP_ICON_DISPLAY_ON);
		    	}
		    	lpImage = icon_image[AP_ICON_NO_SIGNED];
		    	if(lpImage)
		    	{
		    		XM_ImageBlendToFrameBuffer (
		    					lpImage, 
		    					0, 0,
		    					lpImage->width, lpImage->height,
		    					framebuffer,
		    					(unsigned int)(480),
		    					(unsigned int)(280)
		    					);
		    	}
			}
			break;

		case CH_V_AHD12:
			if(rxchip_get_ch1_signal_status()==1)//无信号
			{
		        if((icon_image[AP_ICON_NO_SIGNED] == NULL) || (icon_state[AP_ICON_NO_SIGNED] == AP_ICON_DISPLAY_OFF))
		    	{
		    		icon_state[AP_ICON_NO_SIGNED] = AP_ICON_DISPLAY_ON;
		    		load_image(AP_ICON_NO_SIGNED, AP_ICON_DISPLAY_ON);
		    	}
		    	lpImage = icon_image[AP_ICON_NO_SIGNED];
		    	if(lpImage)
		    	{
		    		XM_ImageBlendToFrameBuffer (
		    					lpImage, 
		    					0, 0,
		    					lpImage->width, lpImage->height,
		    					framebuffer,
		    					(unsigned int)(256-32),
		    					(unsigned int)(280)
		    					);
		    	}
			}

			if(rxchip_get_ch2_signal_status()==1)//无信号
			{
		        if((icon_image[AP_ICON_NO_SIGNED] == NULL) || (icon_state[AP_ICON_NO_SIGNED] == AP_ICON_DISPLAY_OFF))
		    	{
		    		icon_state[AP_ICON_NO_SIGNED] = AP_ICON_DISPLAY_ON;
		    		load_image(AP_ICON_NO_SIGNED, AP_ICON_DISPLAY_ON);
		    	}
		    	lpImage = icon_image[AP_ICON_NO_SIGNED];
		    	if(lpImage)
		    	{
		    		XM_ImageBlendToFrameBuffer (
		    					lpImage, 
		    					0, 0,
		    					lpImage->width, lpImage->height,
		    					framebuffer,
		    					(unsigned int)(256+512-32),
		    					(unsigned int)(280)
		    					);
		    	}
			}
			break;

		case CH_V_AHD21:
			if(rxchip_get_ch1_signal_status()==1)//无信号
			{
		        if((icon_image[AP_ICON_NO_SIGNED] == NULL) || (icon_state[AP_ICON_NO_SIGNED] == AP_ICON_DISPLAY_OFF))
		    	{
		    		icon_state[AP_ICON_NO_SIGNED] = AP_ICON_DISPLAY_ON;
		    		load_image(AP_ICON_NO_SIGNED, AP_ICON_DISPLAY_ON);
		    	}
		    	lpImage = icon_image[AP_ICON_NO_SIGNED];
		    	if(lpImage)
		    	{
		    		XM_ImageBlendToFrameBuffer (
		    					lpImage, 
		    					0, 0,
		    					lpImage->width, lpImage->height,
		    					framebuffer,
		    					(unsigned int)(256+512-32),
		    					(unsigned int)(280)
		    					);
		    	}
			}

			if(rxchip_get_ch2_signal_status()==1)//无信号
			{
		        if((icon_image[AP_ICON_NO_SIGNED] == NULL) || (icon_state[AP_ICON_NO_SIGNED] == AP_ICON_DISPLAY_OFF))
		    	{
		    		icon_state[AP_ICON_NO_SIGNED] = AP_ICON_DISPLAY_ON;
		    		load_image(AP_ICON_NO_SIGNED, AP_ICON_DISPLAY_ON);
		    	}
		    	lpImage = icon_image[AP_ICON_NO_SIGNED];
		    	if(lpImage)
		    	{
		    		XM_ImageBlendToFrameBuffer (
		    					lpImage, 
		    					0, 0,
		    					lpImage->width, lpImage->height,
		    					framebuffer,
		    					(unsigned int)(256-32),
		    					(unsigned int)(280)
		    					);
		    	}
			}
			break;
			
		case CH_H_AHD12:
			if(rxchip_get_ch1_signal_status()==1)//无信号
			{
		        if((icon_image[AP_ICON_NO_SIGNED] == NULL) || (icon_state[AP_ICON_NO_SIGNED] == AP_ICON_DISPLAY_OFF))
		    	{
		    		icon_state[AP_ICON_NO_SIGNED] = AP_ICON_DISPLAY_ON;
		    		load_image(AP_ICON_NO_SIGNED, AP_ICON_DISPLAY_ON);
		    	}
		    	lpImage = icon_image[AP_ICON_NO_SIGNED];
		    	if(lpImage)
		    	{
		    		XM_ImageBlendToFrameBuffer (
		    					lpImage, 
		    					0, 0,
		    					lpImage->width, lpImage->height,
		    					framebuffer,
		    					(unsigned int)(480),
		    					(unsigned int)(150)
		    					);
		    	}
			}

			if(rxchip_get_ch2_signal_status()==1)//无信号
			{
		        if((icon_image[AP_ICON_NO_SIGNED] == NULL) || (icon_state[AP_ICON_NO_SIGNED] == AP_ICON_DISPLAY_OFF))
		    	{
		    		icon_state[AP_ICON_NO_SIGNED] = AP_ICON_DISPLAY_ON;
		    		load_image(AP_ICON_NO_SIGNED, AP_ICON_DISPLAY_ON);
		    	}
		    	lpImage = icon_image[AP_ICON_NO_SIGNED];
		    	if(lpImage)
		    	{
		    		XM_ImageBlendToFrameBuffer (
		    					lpImage, 
		    					0, 0,
		    					lpImage->width, lpImage->height,
		    					framebuffer,
		    					(unsigned int)(480),
		    					(unsigned int)(450)
		    					);
		    	}
			}
			break;

		case CH_H_AHD21:
			if(rxchip_get_ch1_signal_status()==1)//无信号
			{
		        if((icon_image[AP_ICON_NO_SIGNED] == NULL) || (icon_state[AP_ICON_NO_SIGNED] == AP_ICON_DISPLAY_OFF))
		    	{
		    		icon_state[AP_ICON_NO_SIGNED] = AP_ICON_DISPLAY_ON;
		    		load_image(AP_ICON_NO_SIGNED, AP_ICON_DISPLAY_ON);
		    	}
		    	lpImage = icon_image[AP_ICON_NO_SIGNED];
		    	if(lpImage)
		    	{
		    		XM_ImageBlendToFrameBuffer (
		    					lpImage, 
		    					0, 0,
		    					lpImage->width, lpImage->height,
		    					framebuffer,
		    					(unsigned int)(480),
		    					(unsigned int)(450)
		    					);
		    	}
			}

			if(rxchip_get_ch2_signal_status()==1)//无信号
			{
		        if((icon_image[AP_ICON_NO_SIGNED] == NULL) || (icon_state[AP_ICON_NO_SIGNED] == AP_ICON_DISPLAY_OFF))
		    	{
		    		icon_state[AP_ICON_NO_SIGNED] = AP_ICON_DISPLAY_ON;
		    		load_image(AP_ICON_NO_SIGNED, AP_ICON_DISPLAY_ON);
		    	}
		    	lpImage = icon_image[AP_ICON_NO_SIGNED];
		    	if(lpImage)
		    	{
		    		XM_ImageBlendToFrameBuffer (
		    					lpImage, 
		    					0, 0,
		    					lpImage->width, lpImage->height,
		    					framebuffer,
		    					(unsigned int)(480),
		    					(unsigned int)(150)
		    					);
		    	}
			}
			break;

			
		default:
			break;
	}

}

// 将可视的ICON显示在视频层
// 定义三组ICON 16X16 24X24及 32X32
void XM_IconDisplay (xm_osd_framebuffer_t framebuffer)
{
	// 显示不同的ICON排列策略

	// 锁定线程竞争资源
	if(!XM_WaitSemaphore(icon_semaphore))
		return;
   
	if(icon_scheme.icon_scheme == ICON_SCHEME_1)
	{
		if(XMSYS_H264CodecGetMode() != XMSYS_H264_CODEC_MODE_PLAY)
		{
			// 非播放模式
			if(CurrentPhotoMode==0)
			{
				if(get_takephoto_flag()==TRUE)
				{
					video_takephoto_display(framebuffer);//拍照动作图标
				}
				video_ch_display(framebuffer);//视频叠加通道标志信息
				video_splitline_display(framebuffer);//视频叠加分割线
				video_nosignal_display(framebuffer);//视频叠加无信号标志
				Show_recoder_time(framebuffer);//显示已录制时间
				show_local_time(framebuffer);//左下角，嵌入当前时间 小时:分钟
				show_red_dot(framebuffer);//录像红点
				show_sd_status(framebuffer);
				show_mode_icon(framebuffer);//模式图标显示
				//display_record_state_icons_scheme_1 (framebuffer);//录像
				#ifdef JLINK_DISABLE
				show_rx_parameter(framebuffer);
				#endif
			}
		    else
		    {
                 //display_photo_state_icons_scheme_1 (framebuffer);//拍照
			}
		}
		else
		{
				// 播放模式
		}
	}
	
	// 解锁
	XM_SignalSemaphore (icon_semaphore);
}

