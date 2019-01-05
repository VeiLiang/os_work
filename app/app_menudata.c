//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_menudata.c
//	  菜单数据结构定义
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************
#include "rtos.h"
#include <xm_assert.h>
#include <string.h>
#include <xm_printf.h>
#include "xm_app_menudata.h"
#include <xm_icon_manage.h>
#include "xm_arkn141_isp_ae.h"
#include "xm_core.h"
#include "types.h"

extern void HW_LCD_ROTATE(int select);
extern void reset_battery_dec_state(void);



APPMENUDATA AppMenuData;

static const u32_t crc32_table[] =
{
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

/**
 * Calculate the CRC32 value of a memory buffer.
 *
 * @param crc accumulated CRC32 value, must be 0 on first call
 * @param buf buffer to calculate CRC32 value for
 * @param size bytes in buffer
 *
 * @return calculated CRC32 value
 */
u32_t ef_calc_crc32(u32_t crc, const void *buf, u32_t size)
{
    const u8_t *p;

    p = (const u8_t *)buf;
    crc = crc ^ ~0U;

    while (size--) {
		//XM_printf(">>>>*p:%x\r\n", *p);
        crc = crc32_table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    }

    return (crc ^ ~0U);
}

int	AP_VerifyMenuData (APPMENUDATA *lpMenuData)
{
#if 0
	// 根据厂家定制
#if CZS_USB_01 || HONGJING_CVBS
	lpMenuData->record_delay = AP_SETTING_VIDEO_DELAY_0S;			// 任何情况下开机即录像
	lpMenuData->lcd = AP_SETTING_LCD_NEVERCLOSE;		// 无显示屏
	lpMenuData->mic = AP_SETTING_MIC_ON;
#endif

#if SENSOR_BG0806_DEBUG || SENSOR_AR0238_DEBUG
	lpMenuData->video_resolution = AP_SETTING_VIDEORESOLUTION_1080P_30; 	// sensor测试使用
#endif

#if TULV
	lpMenuData->record_delay = AP_SETTING_VIDEO_DELAY_0S;			// 任何情况下开机即录像
	lpMenuData->lcd = AP_SETTING_LCD_NEVERCLOSE;		// 无显示屏
#endif
#endif
	return 1;
}


// 从Flash载入菜单设置
// 1 加载成功
// 0 加载失败
int	AP_LoadMenuData (APPMENUDATA *lpMenuData)
{
	int i;
	APPMENUDATA MenuData;
	u32_t crc = 0;
	
	if(lpMenuData == NULL)
		return 0;

	memset (lpMenuData, 0, sizeof(MenuData));
	XM_printf(">>>>>>APPMENUDATA:%d\r\n", sizeof(APPMENUDATA));
	
	XM_ASSERT(sizeof(APPMENUDATA) <= 512);

	memset (&MenuData, 0, sizeof(MenuData));

	XM_LoadMenuData(&MenuData, sizeof(APPMENUDATA));
	XM_printf(">>>>AP_LoadMenuData, read MenuData.crc32_checksum:%x\r\n", MenuData.crc32_checksum);

	// 检查数据格式
	if(MenuData.menu_id != AP_MENU_DATA_ID)
	{
	    XM_printf("MenuData.menu_id is %2x\n",MenuData.menu_id);
	    return 1;
	}
	
	if(MenuData.menu_version == AP_MENU_DATA_VERSION)
	{
		crc = ef_calc_crc32(crc, (&MenuData), sizeof(APPMENUDATA)-4);//去掉最后crc32的4个字节

		XM_printf(">>>>AP_LoadMenuData, read crc:%x, MenuData.crc32_checksum:%x\r\n", crc, MenuData.crc32_checksum);
		
		if(crc != MenuData.crc32_checksum)
		{
			XM_printf(">>>>>>>>>>>use default value...............\r\n");
			AP_RestoreMenuData();// 构建缺省值
		}
		else
		{
			memcpy (lpMenuData, &MenuData, sizeof(APPMENUDATA));
			AP_VerifyMenuData (lpMenuData);
			XM_printf(">>>>>>>>>>>AP_LoadMenuData, parameter load success...............\r\n");
		}
	}
	else
	{
	    XM_printf(">>>no vaild MenuData.menu_version is %2x\n",MenuData.menu_version);// 无效版本
	    AP_RestoreMenuData();// 构建缺省值
	}

	return 1;
}

// 将菜单设置写入Flash
// 1 保存成功
//	0 保存失败
int	AP_SaveMenuData (APPMENUDATA *lpMenuData)
{
	int i;
	u32_t checksum = 0;
	u32_t crc = 0;
	
	APPMENUDATA tempdata;

	if(lpMenuData == NULL)
		return 0;
	
	lpMenuData->menu_id = AP_MENU_DATA_ID;
	lpMenuData->menu_size = sizeof(APPMENUDATA);
	lpMenuData->menu_version = AP_MENU_DATA_VERSION;
	lpMenuData->menu_checksum = 0;

	crc = ef_calc_crc32(crc, lpMenuData, sizeof(APPMENUDATA)-4);
	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>AP_SaveMenuData,crc:%x\r\n", crc);
	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>AP_SaveMenuData,lpMenuData->crc32_checksum:%x\r\n", lpMenuData->crc32_checksum);
	
	if(crc!=lpMenuData->crc32_checksum)
	{//不同说明数据有变，进行falsh写操作
		lpMenuData->crc32_checksum = crc;
		if(XM_SaveMenuData(lpMenuData, sizeof(APPMENUDATA)) == 0)
		{
			//XM_printf(">>>>write protection bit....\r\n");
			//SpiSetMemoryProtection(0x00);
			//AP_LoadMenuData(&tempdata);
			return 1;
		}
		else
		{
			return 0;
		}
	}
	XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>do not write flash................\r\n");
	return 1;
}


void APP_SaveMenuData(void)
{
    APPMENUDATA *lpMenuData = &AppMenuData;
    AP_SaveMenuData(lpMenuData);
}

int AP_RestoreMenuData (void)
{
	int ret;
	APPMENUDATA *lpMenuData = &AppMenuData;
	OS_EnterRegion();
	memset (&AppMenuData, 0, sizeof(AppMenuData));

	// 构建缺省值
	lpMenuData->mic = AP_SETTING_MIC_OFF;	// 缺省隐私保护
	lpMenuData->lcd = AP_SETTING_LCD_NEVERCLOSE;
	lpMenuData->key = AP_SETTING_KEY_OFF;
	lpMenuData->lang = AP_SETTING_LANG_CHINESE_SIMPLIFIED;
	lpMenuData->day_night = AP_SETTING_DAY_MODE;
	lpMenuData->video_lock = 0;
	lpMenuData->photo = AP_SETTING_PHOTO;
	lpMenuData->sys_setting=AP_SYS_SETTING;
	lpMenuData->video_switch=AP_VIDEO_SWITCH;
	lpMenuData->photograph=AP_PHOTOGRAPH;
	lpMenuData->record_switch=AP_RECORD_SWITCH;
	lpMenuData->record_list=AP_RECORD_LIST;
	lpMenuData->mic_switch=AP_MIC_SWITCH;
	lpMenuData->voice_prompts = AP_SETTING_VOICE_PROMPTS_ON;	// 语音提示开启
	lpMenuData->video_time_size = AP_SETTING_RECORDTIME_3M;		// 3分钟分段
	lpMenuData->recordtime_alarm = 0;	
	lpMenuData->collision_sensitivity = AP_SETTING_COLLISION_3G;   //缺省灵敏度为中
	lpMenuData->parkmonitor = AP_SETTING_PARK_MONITOR_ON;

	lpMenuData->time_stamp = AP_SETTING_VIDEO_TIMESTAMP_ON;
	lpMenuData->flag_stamp = 0;
	lpMenuData->flag_stamp_update = 0;
	lpMenuData->navi_stamp = 0;
	lpMenuData->record_delay = AP_SETTING_VIDEO_DELAY_0S;			// 缺省上电录像

	lpMenuData->urgent_record_time = AP_SETTING_URGENTRECORDTIME_5M;
	lpMenuData->demo_mode = 0;

	
	lpMenuData->video_resolution = AP_SETTING_VIDEORESOLUTION_720P_30;//AP_SETTING_VIDEORESOLUTION_1080P_30; 	// 1080P 30fps

	lpMenuData->mic_volume = 0;
	lpMenuData->bell_volume = 0;

	
	lpMenuData->date_label = AP_SETTING_DATE_LABEL_OFF;


	lpMenuData->voiceassistant_drive = 0;					// 驾驶语音提示 (疲劳驾驶)
	lpMenuData->voiceassistant_battery_alarm = 0;			// 电池电量语音提示
	lpMenuData->voiceassistant_card_status = 0;			// 数据卡状态语音提示
	lpMenuData->voiceassistant_mic_onoff = 0;				// 录音开、关状态语音提示
	lpMenuData->voiceassistant_rec_onoff = 0;				// 录像状态语音提示 (录像开、关、录像模式)
	lpMenuData->voiceassistant_urgent_record = 0;			// 紧急录像启动、关闭语音提示
	lpMenuData->voiceassistant_storage_space = 0;			// 循环录制空间低语音提示
	lpMenuData->voiceassistant_navigate_status = 0;		// 导航状态语音提示

	lpMenuData->cvbs_type = AP_SETTING_CVBS_NTSC;							// cvbs制式选择
	lpMenuData->exposure_compensation = AP_SETTING_EV_0;			// Ev 曝光补偿, -3,~ + 3
	lpMenuData->light_freq=AP_SETTING_LIGHTFREQ_CLOSE;
	lpMenuData->sharpening = AP_SETTING_SHARPENLING_SOFT;							// 锐化强度值
	lpMenuData->video_quality = AP_SETTING_VIDEO_QUALITY_GOOD;						// 视频编码质量(正常, 较好, 高)
    lpMenuData->NoSigned = 0;
    lpMenuData->LCD_Rotate = 0;
    lpMenuData->auto_brightness = 0;
    lpMenuData->blue_show = 0;
    lpMenuData->guide_camer1 = 0;
    lpMenuData->guide_camer2 = 0;
    lpMenuData->guide_camer3 = 0;
    lpMenuData->guide_camer4 = 0;
    lpMenuData->camera1 = 0;
    lpMenuData->camera2 =0;
    lpMenuData->camera3 =0;
    lpMenuData->camera4 =0;
    lpMenuData->camera_mot = 0;
    lpMenuData->carbackdelay = 0;
    lpMenuData->poweron_brightness =0;
    lpMenuData->AHD_ChannelNum =0;
    lpMenuData->VolOnOff =0;
    lpMenuData->LogoEnable =1;
    lpMenuData->AutoDimOnOff =0;
    lpMenuData->Middle_RED_LIne =0;
    lpMenuData->Audio_PWM =0;
    lpMenuData->VCOM_PWM = 12;
    
    lpMenuData->Color_Brightness =50;
    lpMenuData->Color_Contrast =50;
    lpMenuData->Color_Saturation =50;
    lpMenuData->Color_Tone =50;
    
    lpMenuData->AHD_Select =0;
    lpMenuData->Red_Location =0;
    lpMenuData->AUTO_Switch_OnOff =0;
    lpMenuData->PowerOn_Memory =0;
    lpMenuData->PowerOff_Restore =0;

	lpMenuData->curch = CH_V_AHD12;//当前通道
	lpMenuData->prech = CH_V_AHD12;
    lpMenuData->ch1_delay=0;
    lpMenuData->ch2_delay=0;
    lpMenuData->ch3_delay=0;
    lpMenuData->ch4_delay=0;
	lpMenuData->power_state=POWER_STATE_ON;
	lpMenuData->update_date_falg = 0;
	lpMenuData->parking_line = 1;//倒车线默认打开
	lpMenuData->power_mode = POWER_MODE_ON;
	
	ret = AP_SaveMenuData (lpMenuData);
	OS_LeaveRegion();

	return ret;
}

//获取菜单选项设置
BOOL Close_Audio_Sound = TRUE;
unsigned int AP_GetMenuItem (int menu_item)
{
     int i=0;
	if(menu_item >= APPMENUITEM_COUNT)
	{
		XM_printf ("AP_GetMenuItem item=%d exceed maximum item count %d\n", menu_item, APPMENUITEM_COUNT);
		return 0;
	}

	switch (menu_item)
	{
		case APPMENUITEM_MIC:					return AppMenuData.mic;
		case APPMENUITEM_LCD:					return AppMenuData.lcd;
		case APPMENUITEM_MIRROR:                return AppMenuData.mirror;
		case APPMENUITEM_KEY:					return AppMenuData.key;
		case APPMENUITEM_LANG:					return AppMenuData.lang;
		case APPMENUITEM_VOICE_PROMPTS:         return AppMenuData.voice_prompts;
		case APPMENUITEM_VIDEO_TIME_SIZE:		return AppMenuData.video_time_size;
		case APPMENUITEM_VIDEO_RESOLUTION:		return AppMenuData.video_resolution;
		case APPMENUITEM_DATE_LABEL:			return AppMenuData.date_label;
		case APPMENU_IMAGE_QUALITY:        		return AppMenuData.video_quality;
		case APPMENUITEM_COLLISION_SENSITIVITY:	return AppMenuData.collision_sensitivity;
		case APPMENUITEM_TIME_STAMP:			return AppMenuData.time_stamp;
		case APPMENUITEM_NAVI_STAMP:			return AppMenuData.navi_stamp;
		case APPMENUITEM_FLAG_STAMP:			return AppMenuData.flag_stamp;
		case APPMENUITEM_LOCK_MODE:       		i=XMSYS_H264CodecGetVideoLockState();return  i;
        case APPMENUITEM_PHOTO:           	   	return AppMenuData.photo;
		case APPMENUITEM_SYS_SETTING:          	return AppMenuData.sys_setting;
	    case APPMENUITEM_VIDEO_SWITCH:         	return AppMenuData.video_switch;
		case APPMENUITEM_PHOTOGRAPH:           	return AppMenuData.photograph;
		case APPMENUITEM_RECORD_SWITCH:        	return AppMenuData.record_switch;
		case APPMENUITEM_RECORD_LIST:          	return AppMenuData.record_list;
		case APPMENUITEM_MIC_SWITCH:           	return AppMenuData.mic_switch;
		case APPMENUITEM_DAY_NIGHT_MODE:       	return AppMenuData.day_night;
		case APPMENUITEM_PARKMONITOR:          	return  AppMenuData.parkmonitor;
		case APPMENUITEM_FLAG_STAMP_UPDATE:
		{
			unsigned int ret = AppMenuData.flag_stamp_update;
			AppMenuData.flag_stamp_update = 0;
			return ret;
		}
		case APPMENUITEM_RECORD_DELAY:					return AppMenuData.record_delay;
		case APPMENUITEM_URGENT_RECORD_TIME:			return AppMenuData.urgent_record_time;
		case APPMENUITEM_MIC_VOLUME:					return AppMenuData.mic_volume;
		case APPMENUITEM_BELL_VOLUME:					return AppMenuData.bell_volume;
		case APPMENUITEM_VOICEASSISTANT_CARD_STATUS:	return AppMenuData.voiceassistant_card_status;
		case APPMENUITEM_CVBS_TYPE:						return AppMenuData.cvbs_type;
		case APPMENUITEM_EV:							return AppMenuData.exposure_compensation;
        case APPMENUITEM_LIGHT_FREQ:      				return AppMenuData.light_freq;
		case APPMENUITEM_SHARPENING:		    		return AppMenuData.sharpening;
		case APPMENUITEM_VIDEO_QUALITY:					return AppMenuData.video_quality;
		case APPMENUITEM_PARKING_LINE:                  return AppMenuData.parking_line;
		case APPMENUITEM_PARKING_DELAY:                 return AppMenuData.parking_delay;
		case APPMENUITEM_CH:							return AppMenuData.curch;
		case APPMENUITEM_REPLAY_CH:						return AppMenuData.replay_ch;
        case APPMENUITEM_PRE_CH:                        return AppMenuData.prech;
        case APPMENUITEM_CH1_DELAY:                     return AppMenuData.ch1_delay;
        case APPMENUITEM_CH2_DELAY:                     return AppMenuData.ch2_delay;
        case APPMENUITEM_CH3_DELAY:                     return AppMenuData.ch3_delay;
        case APPMENUITEM_CH4_DELAY:                     return AppMenuData.ch4_delay;
        case APPMENUITEM_POWER_STATE:                   return AppMenuData.power_state;
		case APPMENUITEM_POWER_MODE:					return AppMenuData.power_mode;
		
		default:										return 0;
	}
}

// 重新设置LCDC的自动关屏时间
// 0xFFFFFFFF 表示永不关屏
// 其他值 表示自动关屏的时间(毫秒),
void hw_backlight_set_auto_off_ticket (unsigned int ticket);


// 设置菜单选项
void AP_SetMenuItem (int menu_item, unsigned int item_value)
{
	if(menu_item >= APPMENUITEM_COUNT)
	{
		XM_printf ("AP_SetMenuItem item=%d exceed maximum item count %d\n", menu_item, APPMENUITEM_COUNT);
		return;
	}

	switch (menu_item)
	{
		case APPMENUITEM_MIC:
			if(item_value >= AP_SETTING_MIC_OPTION_COUNT)
			{
				XM_printf ("AP_SetMenuItem APPMENUITEM_MIC value=%d exceed maximum option count %d\n",
					item_value, AP_SETTING_MIC_OPTION_COUNT);
				return;
			}
			AppMenuData.mic = (u8_t)item_value;
			break;

		case APPMENUITEM_LCD:
		{
			unsigned int ticket_to_close;
			if(item_value >= AP_SETTING_LCD_OPTION_COUNT)
			{
				XM_printf ("AP_SetMenuItem APPMENUITEM_LCD value=%d exceed maximum option count %d\n",
					item_value, AP_SETTING_LCD_OPTION_COUNT);
				return;
			}
			AppMenuData.lcd = (u8_t)item_value;
			if(AppMenuData.lcd == AP_SETTING_LCD_NEVERCLOSE)
			{
				ticket_to_close = (unsigned int)(-1);
			}
			else if(AppMenuData.lcd == AP_SETTING_LCD_1MIN)
			{
				ticket_to_close = 1*60*1000;
		    }
			else if(AppMenuData.lcd == AP_SETTING_LCD_3MIN)
			{
				ticket_to_close = 3*60*1000;
			}	
			else if(AppMenuData.lcd == AP_SETTING_LCD_5MIN)
			{
				ticket_to_close = 5*60*1000;
		    }
			//else if(AppMenuData.lcd == AP_SETTING_LCD_120S)
			//	ticket_to_close = 120*1000;
            else
            {
		        ticket_to_close = 300*1000;
		    }
			hw_backlight_set_auto_off_ticket (ticket_to_close);
			break;
		}

        case APPMENUITEM_MIRROR:
            AppMenuData.mirror = (u8_t)item_value;
            break;

		case APPMENUITEM_KEY:
			if(item_value >= AP_SETTING_KEY_OPTION_COUNT)
			{
				XM_printf ("AP_SetMenuItem APPMENUITEM_KEY value=%d exceed maximum option count %d\n",
					item_value, AP_SETTING_KEY_OPTION_COUNT);
				return;
			}
			AppMenuData.key = (u8_t)item_value;
			break;

		case APPMENUITEM_LANG:
			if(item_value != AP_SETTING_LANG_CHINESE_SIMPLIFIED)
			{
				// DEMO版本，仅简体中文支持
				XM_printf ("AP_SetMenuItem APPMENUITEM_LANG value=%d, only AP_SETTING_LANG_CHINESE_SIMPLIFIED supported(DEMO)\n",
					item_value);
				return;
			}
			if(item_value >= AP_SETTING_LANG_OPTION_COUNT)
			{
				XM_printf ("AP_SetMenuItem APPMENUITEM_LANG value=%d exceed maximum option count %d\n",
					item_value, AP_SETTING_LANG_OPTION_COUNT);
				return;
			}
			AppMenuData.lang = (u8_t)item_value;
			break;

		case APPMENUITEM_VOICE_PROMPTS:
			if(item_value >= AP_SETTING_VOICE_PROMPTS_OPTION_COUNT)
			{
				XM_printf ("AP_SetMenuItem APPMENUITEM_VOICE_PROMPTS value=%d exceed maximum option count %d\n",
					item_value, AP_SETTING_VOICE_PROMPTS_OPTION_COUNT);
				return;
			}
			AppMenuData.voice_prompts = (u8_t)item_value;
			break;


		case APPMENUITEM_VIDEO_TIME_SIZE:
			if(item_value >= AP_SETTING_RECORDTIME_OPTION_COUNT)
			{
				XM_printf ("AP_SetMenuItem APPMENUITEM_VIDEO_TIME_SIZE value=%d exceed maximum option count %d\n",
					item_value, AP_SETTING_RECORDTIME_OPTION_COUNT);
				return;
			}
			AppMenuData.video_time_size = (u8_t)item_value;
			break;

		case APPMENUITEM_COLLISION_SENSITIVITY:
			if(item_value >= AP_SETTING_COLLISION_OPTION_COUNT)
			{
				XM_printf ("AP_SetMenuItem APPMENUITEM_COLLISION_SENSITIVITY value=%d exceed maximum option count %d\n",
					item_value, AP_SETTING_COLLISION_OPTION_COUNT);
				return;
			}
			AppMenuData.collision_sensitivity = (u8_t)item_value;
			break;

		case APPMENUITEM_TIME_STAMP:
			if(item_value >= AP_SETTING_VIDEO_TIMESTAMP_OPTION_COUNT)
			{
				XM_printf ("AP_SetMenuItem APPMENUITEM_TIME_STAMP value=%d exceed maximum option count %d\n",
					item_value, AP_SETTING_VIDEO_TIMESTAMP_OPTION_COUNT);
				return;
			}
			AppMenuData.time_stamp = (u8_t)item_value;
			break;

		case APPMENUITEM_NAVI_STAMP:
			if(item_value >= AP_SETTING_VIDEO_TIMESTAMP_OPTION_COUNT)
			{
				XM_printf ("AP_SetMenuItem APPMENUITEM_NAVI_STAMP value=%d exceed maximum option count %d\n",
					item_value, AP_SETTING_VIDEO_TIMESTAMP_OPTION_COUNT);
				return;
			}
			AppMenuData.navi_stamp = (u8_t)item_value;
			break;

		case APPMENUITEM_FLAG_STAMP:
			if(item_value >= AP_SETTING_VIDEO_TIMESTAMP_OPTION_COUNT)
			{
				XM_printf ("AP_SetMenuItem APPMENUITEM_FLAG_STAMP value=%d exceed maximum option count %d\n",
					item_value, AP_SETTING_VIDEO_TIMESTAMP_OPTION_COUNT);
				return;
			}
			AppMenuData.flag_stamp = (u8_t)item_value;
			break;

		case APPMENUITEM_FLAG_STAMP_UPDATE:
			AppMenuData.flag_stamp_update = (u8_t)item_value;
			break;

        case APPMENUITEM_PARKMONITOR:
			if(item_value >= AP_SETTING_PARK_MONITOR_OPTION_COUNT)
			{
				XM_printf ("AP_SetMenuItem APPMENUITEM_MONITOR value=%d exceed maximum option count %d\n",
					item_value, AP_SETTING_PARK_MONITOR_OPTION_COUNT);
				return;
			}
			AppMenuData.parkmonitor = (u8_t)item_value;
			
            reset_battery_dec_state();  //重置电量检测状态
			break;

		case APPMENUITEM_RECORD_DELAY:
			if(item_value >= AP_SETTING_VIDEO_DELAY_OPTION_COUNT)
			{
				XM_printf ("AP_SetMenuItem APPMENUITEM_RECORD_DELAY value=%d exceed maximum option count %d\n",
					item_value, AP_SETTING_VIDEO_DELAY_OPTION_COUNT);
				return;
			}
			AppMenuData.record_delay = (u8_t)item_value;
			break;

          case APPMENUITEM_DAY_NIGHT_MODE:
		  	if(item_value >= AP_SETTING_DAY_NIGHT_MODE_OPTION_COUNT)
			{
				XM_printf ("AP_SetMenuItem APPMENUITEM_DAY_NIGHT_MODE value=%d exceed maximum option count %d\n",
					item_value, AP_SETTING_DAY_NIGHT_MODE_OPTION_COUNT);
				return;
			}
			AppMenuData.day_night = (u8_t)item_value;
			break;

		case APPMENUITEM_URGENT_RECORD_TIME:
			if(item_value >= AP_SETTING_URGENTRECORDTIME_COUNT)
			{
				XM_printf ("AP_SetMenuItem APPMENUITEM_URGENT_RECORD_TIME value=%d exceed maximum option count %d\n",
					item_value, AP_SETTING_URGENTRECORDTIME_COUNT);
				return;
			}
			AppMenuData.urgent_record_time = (u8_t)item_value;
			break;

		case APPMENUITEM_MIC_VOLUME:
			if(item_value >= AP_SETTING_SOUNDVOLUME_COUNT)
			{
				XM_printf ("AP_SetMenuItem APPMENUITEM_MIC_VOLUME value=%d exceed maximum option count %d\n",
					item_value, AP_SETTING_SOUNDVOLUME_COUNT);
				return;
			}
			AppMenuData.mic_volume = (u8_t)item_value;
			break;

		case APPMENUITEM_BELL_VOLUME:
			if(item_value >= AP_SETTING_SOUNDVOLUME_COUNT)
			{
				XM_printf ("AP_SetMenuItem APPMENUITEM_BELL_VOLUME value=%d exceed maximum option count %d\n",
					item_value, AP_SETTING_SOUNDVOLUME_COUNT);
				return;
			}
			AppMenuData.bell_volume = (u8_t)item_value;
			break;

		case APPMENUITEM_VIDEO_RESOLUTION:
			if(item_value >= AP_SETTING_VIDEORESOLUTION_COUNT)
			{
				XM_printf ("AP_SetMenuItem AP_SETTING_VIDEORESOLUTION_COUNT value=%d exceed maximum option count %d\n",
					item_value, AP_SETTING_VIDEORESOLUTION_COUNT);
				return;
			}
			AppMenuData.video_resolution = (u8_t)item_value;
			break;


		case APPMENUITEM_VOICEASSISTANT_CARD_STATUS:
			AppMenuData.voiceassistant_card_status = (u8_t)item_value;
			break;

		case APPMENUITEM_CVBS_TYPE:
			if(item_value >= AP_SETTING_CVBS_COUNT)
			{
				XM_printf ("AP_SetMenuItem APPMENUITEM_CVBS_TYPE value=%d exceed maximum option count %d\n",
					item_value, AP_SETTING_CVBS_COUNT);
				return;
			}
			AppMenuData.cvbs_type = (u8_t)item_value;
			break;

		case APPMENUITEM_DATE_LABEL:
			if(item_value >= AP_SETTING_DATE_LABEL_OPTION_COUNT)
			{
				XM_printf ("AP_SetMenuItem APPMENUITEM_DATE_LABEL value=%d exceed maximum option count %d\n",
					item_value, AP_SETTING_DATE_LABEL_OPTION_COUNT);
				return;
			}
			AppMenuData.date_label = (u8_t)item_value;
			break;

		case APPMENU_IMAGE_QUALITY:
			if(item_value >= AP_SETTING_VIDEO_QUALITY_COUNT)
			{
				XM_printf ("AP_SetMenuItem APPMENU_IMAGE_QUALITY value=%d exceed maximum option count %d\n",
					item_value, AP_SETTING_VIDEO_QUALITY_COUNT);
				return;
			}
			AppMenuData.video_quality = (u8_t)item_value;
			break;

		case APPMENUITEM_EV:
		{
			int ev;
			if(item_value >= AP_SETTING_EV_COUNT)
			{
				XM_printf ("AP_SetMenuItem APPMENUITEM_EV value=%d exceed maximum option count %d\n",
					item_value, AP_SETTING_CVBS_COUNT);
				return;
			}

			AppMenuData.exposure_compensation = (u8_t)item_value;
			ev = item_value - AP_SETTING_EV_0;
			xm_arkn141_isp_set_exposure_compensation (ev);
			break;
		}
		case APPMENUITEM_LIGHT_FREQ:
		{
			int ev;
			if(item_value >= AP_SETTING_LIGHTFREQ_COUNT)
			{
				XM_printf ("AP_SetMenuItem APPMENUITEM_LIGHT_FREQ value=%d exceed maximum option count %d\n",
					item_value, AP_SETTING_LIGHTFREQ_COUNT);
				return;
			}
			AppMenuData.light_freq = (u8_t)item_value;
			ev = item_value - AP_SETTING_LIGHTFREQ_CLOSE;
			xm_arkn141_isp_set_flicker_freq(ev);
			break;
		}
		case APPMENUITEM_SHARPENING:
		{
			int sharpening;
			if(item_value >= AP_SETTING_SHARPENLING_COUNT)
			{
				return;
			}
			AppMenuData.sharpening = (u8_t)item_value;
			sharpening = item_value - AP_SETTING_SHARPENLING_MINOR;
			xm_arkn141_isp_set_sharpening_value (sharpening);
			break;
		}

		case APPMENUITEM_VIDEO_QUALITY:
		{
			int video_quality;
			if(item_value >= AP_SETTING_VIDEO_QUALITY_COUNT)
			{
				return;
			}
			AppMenuData.video_quality = (u8_t)item_value;
			break;
		}
		case APPMENUITEM_PARKING_LINE:
		    AppMenuData.parking_line = (u8_t)item_value;
		    break;
		case APPMENUITEM_PARKING_DELAY:
		    AppMenuData.parking_delay = (u8_t)item_value;
		    break;
		case APPMENUITEM_CH:
			AppMenuData.curch = (u8_t)item_value;
			break;

		case APPMENUITEM_REPLAY_CH:
			AppMenuData.replay_ch = (u8_t)item_value;
			break;
        case APPMENUITEM_PRE_CH:
            AppMenuData.prech = (u8_t)item_value;
			break;
        case APPMENUITEM_CH1_DELAY:
            AppMenuData.ch1_delay = (u8_t)item_value;
			break;		
        case APPMENUITEM_CH2_DELAY:
            AppMenuData.ch2_delay = (u8_t)item_value;
			break;	
        case APPMENUITEM_CH3_DELAY:
            AppMenuData.ch3_delay = (u8_t)item_value;
			break;	
        case APPMENUITEM_CH4_DELAY:
            AppMenuData.ch4_delay = (u8_t)item_value;
			break;	
		case APPMENUITEM_POWER_STATE: 
		     AppMenuData.power_state = (u8_t)item_value;
			 break;
		case APPMENUITEM_POWER_MODE: 
		     AppMenuData.power_mode = (u8_t)item_value;
			 break;			 
		default:
			return;
	}
}

// 返回视频记录时间长度(秒)
unsigned int AP_GetVideoTimeSize (void)
{
	unsigned int video_time_size = AP_GetMenuItem(APPMENUITEM_VIDEO_TIME_SIZE);
	
	switch(video_time_size)
	{
		case AP_SETTING_RECORDTIME_1M:
			video_time_size = 1*60;
			break;
			
		case AP_SETTING_RECORDTIME_3M:
			video_time_size = 3*60;
			break;

		case AP_SETTING_RECORDTIME_5M:
			video_time_size = 5*60;
			break;

		case AP_SETTING_RECORDTIME_10M:
			video_time_size = 10*60;
			break;


		default:
			XM_printf ("AP_GetVideoTimeSize illegal video_time_size %d\n", video_time_size);
			video_time_size = 3*60;
			break;
	}

	return video_time_size;
}



static const int record_delay_time[AP_SETTING_VIDEO_DELAY_OPTION_COUNT] = {
	0,	3,	5, 10, 20
};

// 返回"延时记录"选项的时间值(秒)
unsigned int AP_GetRecordDelayTime (void)
{
	if(AppMenuData.record_delay >= AP_SETTING_VIDEO_DELAY_OPTION_COUNT)
		return 0;
	return record_delay_time[AppMenuData.record_delay];
}

//获取自动亮度开关
unsigned int AP_GetAutoBrightNess(void)
{
    return AppMenuData.auto_brightness;
}
//默认为蓝色
unsigned int AP_GetBlue(void)
{
    return AppMenuData.blue_show;
}
//摄像头个数
unsigned int AP_GetAHD_ChannelNum(void)
{
    return AppMenuData.AHD_ChannelNum + 1;
}
//摄像头 延时1
unsigned int AP_GetDelay_Camera1(void)
{
    if(AppMenuData.carbackdelay)
        return AppMenuData.guide_camer1;
    return 0;
}
//摄像头 延时2
unsigned int AP_GetDelay_Camera2(void)
{
    if(AppMenuData.carbackdelay)
        return AppMenuData.guide_camer2;
    return 0;
}
//摄像头 延时3
unsigned int AP_GetDelay_Camera3(void)
{
    if(AppMenuData.carbackdelay)
        return AppMenuData.guide_camer3;
    return 0;
}
//摄像头 延时4
unsigned int AP_GetDelay_Camera4(void)
{
    if(AppMenuData.carbackdelay)
        return AppMenuData.guide_camer4;
    return 0;
}

//开机背光亮度
unsigned int AP_GetBright_Switch(void)
{
    return AppMenuData.poweron_brightness;
}

VOID AP_SetBright_Switch(u8_t Bright_Switch)
{
    AppMenuData.poweron_brightness = Bright_Switch;
}


//开关开机logo
unsigned int AP_GetLogo(void)
{
    return AppMenuData.LogoEnable;
}

//开关红外竖线显示
unsigned int AP_GetMidle_RED_Line(void)
{
    return AppMenuData.Middle_RED_LIne;
}

//摄像头 倒车标识1
unsigned int AP_GetGuide_Camera1(void)
{
    return AppMenuData.camera1;
}

//摄像头 倒车标识2
unsigned int AP_GetGuide_Camera2(void)
{
    return AppMenuData.camera2;
}

//摄像头 倒车标识3
unsigned int AP_GetGuide_Camera3(void)
{
    return AppMenuData.camera3;
}

unsigned int AP_GetCamera_Mot(void)
{
    return AppMenuData.camera_mot;
}

//摄像头 倒车标识4
unsigned int AP_GetGuide_Camera4(void)
{
    return AppMenuData.camera4;
}
//获取声音
unsigned int AP_GetAudo_PWM()
{
    return AppMenuData.Audio_PWM;
}
VOID AP_SetAudo_PWM(int Data_PWM)
{
     AppMenuData.Audio_PWM = Data_PWM;
}

unsigned int AP_GetVCOM_PWM()
{
    return AppMenuData.VCOM_PWM;
}
void AP_SetVCOM_PWM(u8_t Data_PWM)
{
  AppMenuData.VCOM_PWM=Data_PWM;
}

unsigned int AP_GetLCD_Rotate(void)
{
    return AppMenuData.LCD_Rotate;
}

void AP_SetLCD_Rotate(u8_t Lcd_Rotate)
{
    AppMenuData.LCD_Rotate = Lcd_Rotate;
    HW_LCD_ROTATE(Lcd_Rotate);
}

unsigned int AP_GetColor_Brightness(void)
{
    return AppMenuData.Color_Brightness;
}

void AP_SetColor_Brightness(u8_t Brightness)
{
    AppMenuData.Color_Brightness = Brightness;
}

VOID AP_SetCamera_Mot(u8_t mot)
{
    AppMenuData.camera_mot = mot;
}

unsigned int AP_GetColor_Contrast(void)
{
    return AppMenuData.Color_Contrast;
}

VOID AP_SetColor_Contrast(u8_t Contrast)
{
    AppMenuData.Color_Contrast = Contrast;
}

unsigned int AP_GetColor_Saturation(void)
{
    return AppMenuData.Color_Saturation;
}

VOID AP_SetColor_Saturation(u8_t Saturation)
{
    AppMenuData.Color_Saturation = Saturation;
}

unsigned int AP_GetColor_Tone(void)
{
    return AppMenuData.Color_Tone;
}

void AP_SetColor_Tone(u8_t Tone)
{
    AppMenuData.Color_Tone = Tone;
}

VOID Select_Background_Color(VOID)
{

}

unsigned int AP_GetLang()
{
    return AppMenuData.lang;
}

unsigned int AP_GetAHD_Select()
{
    return AppMenuData.AHD_Select;
}

VOID AP_SetAHD_Select(u8_t select)
{
    AppMenuData.AHD_Select = select;
}

unsigned int AP_GetRed_Location()
{
    return AppMenuData.Red_Location;
}

VOID AP_SetRed_Location(int Location)
{
    AppMenuData.Red_Location = Location;
}

u8_t APP_GetAudio_Sound(VOID)
{
    return AppMenuData.voice_prompts;
}

VOID APP_SetAudio_Sound(u8_t Audio_Sound)
{
    AppMenuData.voice_prompts = Audio_Sound;
}

u8_t APP_GetAuto_Switch(VOID)
{
    return AppMenuData.AUTO_Switch_OnOff;
}

VOID APP_SetAuto_Switch(u8_t Auto_switch)
{
    AppMenuData.AUTO_Switch_OnOff = Auto_switch;
}

u8_t APP_GetVOl_OnOff(VOID)
{
    return AppMenuData.VolOnOff;
}

u8_t APP_GetAuto_DimOnOff(VOID)
{
    return AppMenuData.AutoDimOnOff;
}

u8_t APP_GetPowerOn_Memory(VOID)
{
    return AppMenuData.PowerOn_Memory;
}

VOID APP_SetPowerOn_Memory(u8_t PowerOn_Memory)
{
    AppMenuData.PowerOn_Memory = PowerOn_Memory;
}
VOID APP_SetPowerOff_Restore(u8_t PowerOff_Restore)
{
    AppMenuData.PowerOff_Restore = PowerOff_Restore;
}

u8_t APP_GetPowerOff_Restore(VOID)
{
    return AppMenuData.PowerOff_Restore;
}


u8_t APP_GetMemory(VOID)
{
    if((APP_GetPowerOn_Memory() && (AP_GetBright_Switch() == 1)) || ((AP_GetBright_Switch() == 0)&&AppMenuData.PowerOff_Restore))
        return TRUE;
    return FALSE;
}


