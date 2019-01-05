#include <stdio.h>
#include "RTOS.h"		// OSͷ�ļ�
#include "FS.h"

#include "hardware.h"
#include "xm_flash_space_define.h"
#include "xm_core.h"
#include <xm_base.h>
#include <xm_printf.h>
#include <xm_dev.h>
#include <xm_key.h>
#include <assert.h>
#include <stdarg.h>
#include <xm_file.h>
#include <xm_user.h>
#include <hw_osd_layer.h>
#include <watchdog.h>
#include "ssi.h"
#include "lcd.h"
#include "xm_power.h"
#include "xm_videoitem.h"
#include "xm_adc.h"
#include "xm_app_menudata.h"
extern int PostMediaFileListMessage (int ch, int type);


BYTE _core_bTimeSetting = 0;			// ϵͳʱ��δ����

static unsigned int bl_off_time;
static unsigned int bl_off_ticket;
static WORD wAutoPowerOffTime = 0;
static BYTE bVideoContrastLevel = 7;
static BYTE bVoltageLevel = DEVCAP_VOLTAGE_BAD;
static unsigned int hw_bUsbConnectState;

// ȱʡʱ���⿪��
 BYTE bBackLightOn = 1;			// 1 BL ON, 0 BL off
BYTE bSDCardState = DEVCAP_SDCARDSTATE_UNPLUG;			// 0 SD��δ����
										// 1 APP_SYSTEMCHECK_CARD_WRITEPROTECT
										// 2 APP_SYSTEMCHECK_SUCCESS
BYTE bBackupVoltageLevel = DEVCAP_VOLTAGE_BAD;	// ���ݵ��״̬


BYTE bHdmiPlugIn = DEVCAP_SCREEN_DISCONNECT;
BYTE hw_uAvoutType = XM_LCDC_TYPE_CVBS_NTSC;		// ȱʡѡ��NTSC�Ƶ������
//BYTE hw_uAvoutType = XM_LCDC_TYPE_CVBS_PAL;			// ȱʡѡ��PAL�Ƶ������
BYTE hw_bSensorConnect = DEVCAP_SENSOR_CONNECT;
volatile unsigned int hw_bLcdPlugIn = 1;

#if defined(_SUPPORT_TV_NTSC_) || defined( _SUPPORT_TV_PAL_)
volatile unsigned int hw_bAvoutPlugIn = 1;			// ʹ��AVOUT��Ϊ��ʾ���
#else
volatile unsigned int hw_bAvoutPlugIn = 0;			// AVOUT����/�γ�ģ��
#endif

volatile unsigned int hw_bHdmiPlugIn = 0;


volatile BYTE hw_bVideoRec = DEVCAP_VIDEO_REC_STOP;
volatile BYTE hw_bOsdOn = 0;



// ��������LCDC���Զ�����ʱ��
// 0xFFFFFFFF ��ʾ��������
// ����ֵ ��ʾ�Զ�������ʱ��(����),
void hw_backlight_set_auto_off_ticket (unsigned int ticket)
{
	XM_lock ();
	bl_off_ticket = ticket;
	//printf ("BL %d\n", ticket);
	if(bl_off_ticket == (unsigned int)(-1))
	{
		bl_off_time = 0xFFFFFFFF;
	}
	else
	{
		bl_off_time = bl_off_ticket + XM_GetTickCount();
	}
	XM_unlock ();
	//XM_SetFmlDeviceCap (DEVCAP_BACKLIGHT, 1);
}

unsigned int LCD_layer_State (int layer);
void LCD_Layer_Close(int layer );
void LCD_Layer_Open(int layer );

static unsigned int lcdc_osd_state = 0;
static void lcd_set_display_on (void)
{
	if(lcdc_osd_state & (1 << 0))
		LCD_Layer_Open (0);
	if(lcdc_osd_state & (1 << 1))
		LCD_Layer_Open (1);
	if(lcdc_osd_state & (1 << 2))
		LCD_Layer_Open (2);
	lcdc_osd_state = 0;
}

// �ر�LCD��ʾ
static void lcd_set_display_off (void)
{
	// �ر�LCD��ʾ
	unsigned int ticket_timeout;
	if(hw_bLcdPlugIn == 0)	// ����
		return;

	//OS_EnterRegion();
	XM_lock ();
	lcdc_osd_state  = LCD_layer_State (0) << 0;
	lcdc_osd_state |= LCD_layer_State (1) << 1;
	lcdc_osd_state |= LCD_layer_State (2) << 2;
	XM_unlock ();

	LCD_INTR_CLR_reg = 0xFFFFFFFF;
	ticket_timeout = XM_GetTickCount() + 66;		// �����ʱ2֡
	while(!(LCD_STATUS_reg & 0x01))		// bit1: lcd_done_intr(ÿ�������ж�) ,
	{
		if(XM_GetTickCount() >= ticket_timeout)
			break;
	}

	XM_lock ();
	LCD_Layer_Clr( 0 );
	LCD_Layer_Clr( 1 );
	LCD_Layer_Clr( 2 );
	XM_unlock ();

	// �ȴ�OSD�޸�ͬ����OSD�ڲ��Ĵ������
	LCD_cpu_osd_coef_syn_reg = 0x7;
	ticket_timeout = XM_GetTickCount() + 66;		// �����ʱ2֡
	while(LCD_cpu_osd_coef_syn_reg & 0x7)
	{
		if(XM_GetTickCount() >= ticket_timeout)
			break;
	}

	//LCD_Layer_Close (0);
	//LCD_Layer_Close (1);
	//LCD_Layer_Close (2);
	//OS_LeaveRegion();
}

// ����ֵΪ1�������¿���
int backlight_clear_auto_off_ticket (void)
{
	int ret = 0;
	XM_lock ();
	if(bl_off_ticket)
	{
		if(bl_off_ticket == (unsigned int)(-1))
			bl_off_time = 0xFFFFFFFF;
		else
			bl_off_time = bl_off_ticket + XM_GetTickCount();
	}
	XM_unlock ();
	//����ر���.��Ӧ�õȰ�����Ӧ�ſ���
	if(XM_GetFmlDeviceCap(DEVCAP_OSD) == 0)
	{
		// ���ͱ��⿪���¼� ���͸��¼�,������ɰ����¼���ͣ�Ĵ���
		//ret = XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_BL_ON);
		// OSD����
		XM_SetFmlDeviceCap (DEVCAP_OSD, 1);
		// ���⿪��
		XM_SetFmlDeviceCap (DEVCAP_BACKLIGHT, 1);
	}
	return ret;
}

void backlight_check_auto_off (void)
{
	if(bBackLightOn && bl_off_time && (XM_GetTickCount() >= bl_off_time))
	{
		//��������,
		if(XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_BL_OFF))
		{
			// �������Ϣû�д���, 0.5����ط�
			bl_off_time = XM_GetTickCount() + 500;
		}
	}
}

#if TULV
extern void rm68172_screen_poweroff (void);
#endif

VOID XM_ShutDownSystem  (BYTE bShutDownType)
{
	// �ر�UI
	//extern void LCD_Layer_Close(int layer );
	OS_EnterRegion ();	// ��ֹ�����л�
	XM_printf ("ShutDown %d\n", bShutDownType);
	//����˵�����
	//APP_SaveMenuData ();
	// �ر���
	XM_SetFmlDeviceCap (DEVCAP_BACKLIGHT, 0);

	//LCD_Layer_Close (0);
	//LCD_Layer_Close (1);
	//LCD_Layer_Close (2);

	//OS_Delay (30);

#if TULV
	//rm68172_screen_poweroff ();
#endif

	// ��ֹwatchdog
	hw_watchdog_init (0);

	if(bShutDownType == SDS_REBOOT)
	{
	    //������λ����
	    APP_SetPowerOn_Memory(0);
        APP_SetPowerOff_Restore(0);
		//AP_SetBright_Switch(2);
        APP_SaveMenuData ();
		watchdog_reset ();
	}
	else if(bShutDownType == SDS_POWERACC)
	{
		xm_power_unlock ();

		// �ȴ�PowerControl��·���糹�׹ر�
		delay (0x400000);

		if(xm_power_check_usb_on())
		{
			// ���ACC�ѹ���, ��ʱϵͳ��������, ִ��WatchDog��λ, ��������
			watchdog_reset ();
		}
	}
	else //if(bShutDownType == SDS_POWEROFF)
	{
		xm_power_unlock ();
	}

	while(1)
	{
		__asm ("wfi");		// Wait For Interrupt
	}

}


void XM_SetCardState (DWORD dwValue)
{
	// ���ݿ�״̬��UI�̷߳���֪ͨ��Ϣ
	if(dwValue == DEVCAP_SDCARDSTATE_UNPLUG)
	{
		bSDCardState = dwValue;
		// ���γ���Ϣ
	}
	else if(dwValue == DEVCAP_SDCARDSTATE_INSERT)
	{
		bSDCardState = dwValue;
		// ������, �ļ�ϵͳ���OK, ����¼��
	}
	else if(dwValue == DEVCAP_SDCARDSTATE_FS_ERROR)
	{
		bSDCardState = dwValue;
		// ���ļ�ϵͳ�쳣, ���޷�¼��
	}
	else if(dwValue == DEVCAP_SDCARDSTATE_INVALID)
	{
		bSDCardState = dwValue;
		// ����Ч, ���Ӵ�����
	}
}

// ����ϵͳ����
extern BOOL Close_Audio_Sound;
XMBOOL	XM_SetFmlDeviceCap  (BYTE bDevCapIndex, DWORD dwValue)
{
	switch (bDevCapIndex)
	{
		case DEVCAP_AUTOPOWEROFFTIME:
			// ���û��ȡ�Զ��ػ�ʱ��
			// wValue = 0��ʾ��ֹ�Զ��ػ�����0ֵ��ʾ���� (1 ~ 120)
			if(dwValue > 120)
				return 0;
			wAutoPowerOffTime = (WORD)dwValue;
			break;

		case DEVCAP_VIDEOCONTRASTLEVEL:
			// ���û��ȡ�Աȶ� (0 ~ 15��)
			if(dwValue >= 15)
				return 0;
			bVideoContrastLevel = (BYTE)dwValue;
			break;

		case DEVCAP_MAINBATTERYVOLTAGE:
			// ���������õ�ѹ��ֻ�����ȡ
			return 0;

        case DEVCAP_USB:
			hw_bUsbConnectState = (BYTE)dwValue;
			return 0;

		case DEVCAP_BACKLIGHT:
			//if(bBackLightOn != dwValue)
			{
				bBackLightOn = (BYTE)dwValue;
                if(bBackLightOn == 0) { //�رձ���
                    HW_LCD_BackLightOff();
                    AP_SetMenuItem(APPMENUITEM_POWER_STATE,POWER_STATE_OFF);
                }else {//��������
                    //�ǹػ�״̬�´򿪱���
                    if(Close_Audio_Sound)
                    {
                        HW_LCD_BackLightOn();
                        AP_SetMenuItem(APPMENUITEM_POWER_STATE,POWER_STATE_ON);
                    }
                }
                #if 0
				if(dwValue == 0)
					HW_lcdc_set_backlight_on (0, 0.0);
				else
				{
					backlight_clear_auto_off_ticket ();
					HW_lcdc_set_backlight_on (0, 1.0);
				}
                #endif

			}
			return 0;

		case DEVCAP_BACKLIGHT_LEVEL:		// ���⼶������
			if(dwValue > DEVCAP_BACKLIGHT_LEVEL_100)
				dwValue = DEVCAP_BACKLIGHT_LEVEL_100;

			HW_lcdc_set_backlight_on (0, ((float)dwValue) / 100.0);
			break;

		case DEVCAP_SDCARDSTATE:
			// ���ݿ�״̬��UI�̷߳���֪ͨ��Ϣ
			if(dwValue == DEVCAP_SDCARDSTATE_UNPLUG)
			{
				bSDCardState = dwValue;
				// ���γ���Ϣ
				XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_CARD_UNPLUG);
			}
			else if(dwValue == DEVCAP_SDCARDSTATE_INSERT)
			{
				bSDCardState = dwValue;
				// ������, �ļ�ϵͳ���OK, ����¼��
				XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_CARD_INSERT);
			}
			else if(dwValue == DEVCAP_SDCARDSTATE_FS_ERROR)
			{
				bSDCardState = dwValue;
				// ���ļ�ϵͳ�쳣, ���޷�¼��
				XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_CARD_FS_ERROR);
			}
			else if(dwValue == DEVCAP_SDCARDSTATE_INVALID)
			{
				bSDCardState = dwValue;
				// ����Ч, ���Ӵ�����
				XM_KeyEventProc (VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_CARD_INVALID);
			}

			break;

		case DEVCAP_VIDEO_REC:
			if(dwValue == DEVCAP_VIDEO_REC_START || dwValue == DEVCAP_VIDEO_REC_STOP)
			{
				if(XMSYS_H264CodecGetForcedNonRecordingMode())
				{
					hw_bVideoRec= DEVCAP_VIDEO_REC_STOP;
				}
				else
				{
					hw_bVideoRec = dwValue;
				}
				return 1;
			}
			else
				return 0;

		case DEVCAP_OSD:
			hw_bOsdOn = dwValue;
			break;

		case DEVCAP_AVOUT:
#if CZS_USB_01
#else
			hw_bAvoutPlugIn = dwValue;
#endif
			break;

		default:
			return 0;
	}
	return 1;
}

#define	VOLTAGE_LEVEL_COUNT		4		// ��ѹ�ּ�����, �ļ�(WORST, BAD, NORMAL, GOOD)

// �ּ���ѹֵ����
static const unsigned int voltage_value_definition[VOLTAGE_LEVEL_COUNT - 1] =
{
   //  WORST   |      BAD    |  NORMAL   |    GOOD
#if TULV
//	          3900,         8000,        10000,		// 12.0 v
             3600,         3900,        4100,		// 3.30 v
#else
             2400,         2700,        3000,		// 3.30 v
#endif
};

// ��ѹֵ�ȼ����� (4��, ͨ��3����ѹֵ����)
static const unsigned int voltage_level_definition[VOLTAGE_LEVEL_COUNT] =
{
	//     WORST                 BAD                  NORMAL               GOOD
   DEVCAP_VOLTAGE_WORST, DEVCAP_VOLTAGE_BAD, DEVCAP_VOLTAGE_NORMAL, DEVCAP_VOLTAGE_GOOD
};


// ��ȡϵͳ����
DWORD  	XM_GetFmlDeviceCap  (BYTE bDevCapIndex)
{
	switch (bDevCapIndex)
	{
		case DEVCAP_AUTOPOWEROFFTIME:
			return wAutoPowerOffTime;

		case DEVCAP_VIDEOCONTRASTLEVEL:
			return (WORD)bVideoContrastLevel;

        case DEVCAP_USB:
			return hw_bUsbConnectState;

		case DEVCAP_MAINBATTERYVOLTAGE:
		{
			// DEVCAP_VOLTAGE_WORST
			// DEVCAP_VOLTAGE_BAD
			// DEVCAP_VOLTAGE_NORMAL
			// DEVCAP_VOLTAGE_GOOD
			unsigned int level;
			unsigned int voltage = 3;//XM_GetBatteryVoltage ();
			//XM_printf("battery voltage = %d\n",voltage);

			// ��ּ���ѹֵ�Ƚ�
			for (level = 0; level < sizeof(voltage_value_definition)/sizeof(voltage_value_definition[0]); level++)
			{
				if(voltage < voltage_value_definition[level])
					break;
			}
			bVoltageLevel = voltage_level_definition[level];

#if 0
			if(vol >= 3000)	// 3.0v
				bVoltageLevel = DEVCAP_VOLTAGE_GOOD;
			else if(vol >= 2700)	// 2.7v
				bVoltageLevel = DEVCAP_VOLTAGE_NORMAL;
			else if(vol >= 2400)	// 2.4v
				bVoltageLevel = DEVCAP_VOLTAGE_BAD;
			else
				bVoltageLevel = DEVCAP_VOLTAGE_WORST;
#endif

			return bVoltageLevel;
		}

		case DEVCAP_BACKLIGHT:
			return bBackLightOn;

		case DEVCAP_BACKLIGHT_LEVEL:	// ���⼶������
		{
			float level = HW_lcdc_get_backlight_on (0);
			DWORD dw_level = (DWORD)(level * 100.0);
			if(dw_level > DEVCAP_BACKLIGHT_LEVEL_100)
				dw_level = DEVCAP_BACKLIGHT_LEVEL_100;
			return dw_level;
		}

		case DEVCAP_SDCARDSTATE:	// SD��״̬���
			return bSDCardState;

		case DEVCAP_TIMESETTING:	// ϵͳʱ���Ƿ�������
			return _core_bTimeSetting;

		case DEVCAP_BACKUPBATTERYVOLTAGE:	// ���ݵ��
			return bBackupVoltageLevel;

		case DEVCAP_AVOUT:
			return hw_bAvoutPlugIn;

		//case DEVCAP_SCREEN:
		//	return bLcdPlugIn;

		case DEVCAP_AVOUT_TYPE:
			return hw_uAvoutType;

		case DEVCAP_HDMI:
			return bHdmiPlugIn;

		case DEVCAP_LCD:					// LCD
			return hw_bLcdPlugIn;

		case DEVCAP_SCREEN:				//
			if(hw_bAvoutPlugIn || hw_bLcdPlugIn || hw_bHdmiPlugIn)
				return 1;
			else
				return 0;

		case DEVCAP_SENSOR:				//
			return hw_bSensorConnect;

		case DEVCAP_VIDEO_REC:
			return hw_bVideoRec;

		case DEVCAP_OSD:
			return hw_bOsdOn;

		default:
			return 0;
	}
}

// �����˵�����������
int		XM_LoadMenuData (void *lpMenuData, int cbMenuData)
{
	char *data;
	int ret = -1;
	if(lpMenuData == NULL || cbMenuData > XM_FLASH_MENUDATA_SIZE)
	{
		XM_printf ("XM_LoadMenuData Failed, lpMenuData=0x%08x, cbMenuData=0x%08x\n", lpMenuData, cbMenuData);
		return ret;
	}
	data = (char *)kernel_malloc (XM_FLASH_MENUDATA_SIZE);
	if(data == NULL)
	{
		XM_printf ("XM_LoadMenuData Failed, memory busy\n");
		return ret;
	}
	memset (data, 0, XM_FLASH_MENUDATA_SIZE);
	if(Spi_Read (XM_FLASH_MENUDATA_BASE, (unsigned char *)data, XM_FLASH_MENUDATA_SIZE) == 1)
	{
		// �ɹ�
		ret = 0;
	}
	else
	{
		XM_printf ("XM_LoadMenuData Failed, Flash io error\n");
	}
	memcpy (lpMenuData, data, cbMenuData);
	kernel_free (data);
	return ret;
}

// ����˵�����������
int		XM_SaveMenuData (void *lpMenuData, int cbMenuData)
{
	char *data;
	int ret = -1;
	int loop;
	// ����ⲿACC��Դ״��
	if(xm_power_check_acc_safe_or_not() == 0)
	{
		// �ǰ�ȫ��ѹ
		return -1;
	}

	if(lpMenuData == NULL || cbMenuData > XM_FLASH_MENUDATA_SIZE)
	{
		XM_printf ("XM_SaveMenuData Failed, lpMenuData=0x%08x, cbMenuData=0x%08x\n", lpMenuData, cbMenuData);
		return ret;
	}
	data = (char *)kernel_malloc (XM_FLASH_MENUDATA_SIZE);
	if(data == NULL)
	{
		XM_printf ("XM_SaveMenuData Failed, memory busy\n");
		return ret;
	}
	memset (data, 0, XM_FLASH_MENUDATA_SIZE);
	memcpy (data, 	lpMenuData, cbMenuData);
	loop = 3;	// �ظ�д�����3��
	while(loop > 0)
	{
		if(Spi_Write (XM_FLASH_MENUDATA_BASE, (unsigned char *)data, XM_FLASH_MENUDATA_SIZE))
			break;
		loop --;
	}
	if(loop)
	{
		ret = 0;
	}
	else
	{
		XM_printf ("XM_SaveMenuData Failed, Flash io error\n");
	}
	kernel_free (data);
	return ret;
}


#define	WIN32_FILEFIND_MAGIC		0x4D464657 // "WFFM"

void XM_FileTimeToSystemTime (UINT TimeStamp, XMSYSTEMTIME *lpSystemTime)
{
	FS_FILETIME FileTime;
	FS_TimeStampToFileTime(TimeStamp, &FileTime);
	lpSystemTime->wYear = FileTime.Year;
	lpSystemTime->bDay = (BYTE)FileTime.Day;
	lpSystemTime->bMonth = (BYTE)FileTime.Month;
	lpSystemTime->bHour = (BYTE)FileTime.Hour;
	lpSystemTime->bMinute = (BYTE)FileTime.Minute;
	lpSystemTime->bSecond = (BYTE)FileTime.Second;
}

static void GetFileInfo (XMFILEFIND *lpFindFileData)
{
	XMSYSTEMTIME FileTime;
	XM_FileTimeToSystemTime (lpFindFileData->FindData.CreationTime, &FileTime);
	lpFindFileData->CreationTime.wYear = FileTime.wYear;
	lpFindFileData->CreationTime.bDay = (BYTE)FileTime.bDay;
	lpFindFileData->CreationTime.bSecond = (BYTE)FileTime.bSecond;
	lpFindFileData->CreationTime.bMonth = (BYTE)FileTime.bMonth;
	lpFindFileData->CreationTime.bMinute = (BYTE)FileTime.bMinute;
	lpFindFileData->CreationTime.bHour = (BYTE)FileTime.bHour;
	lpFindFileData->CreationTime.bDayOfWeek = (BYTE)FileTime.bDayOfWeek;

	XM_FileTimeToSystemTime (lpFindFileData->FindData.LastWriteTime, &FileTime);
	lpFindFileData->LastWriteTime.wYear = FileTime.wYear;
	lpFindFileData->LastWriteTime.bDay = (BYTE)FileTime.bDay;
	lpFindFileData->LastWriteTime.bSecond = (BYTE)FileTime.bSecond;
	lpFindFileData->LastWriteTime.bMonth = (BYTE)FileTime.bMonth;
	lpFindFileData->LastWriteTime.bMinute = (BYTE)FileTime.bMinute;
	lpFindFileData->LastWriteTime.bHour = (BYTE)FileTime.bHour;
	lpFindFileData->LastWriteTime.bDayOfWeek = (BYTE)FileTime.bDayOfWeek;

	lpFindFileData->dwFileAttributes = lpFindFileData->FindData.Attributes;;
	lpFindFileData->nFileSize = lpFindFileData->FindData.FileSize;
}

XMBOOL 	XM_FindFirstFile(char* lpFileName, XMFILEFIND *lpFindFileData, char *lpFileNameBuffer, int cbFileNameBuffer)
{
	// ����ļ�ϵͳ�Ƿ����
	/*
	if(FS_GetVolumeStatus("") != FS_MEDIA_IS_PRESENT)
	{
		return 0;
	}
	*/

	if(lpFileName == NULL || *lpFileName == 0)
		return 0;
	if(lpFindFileData == NULL)
		return 0;
	if(lpFileNameBuffer == NULL || cbFileNameBuffer < XM_MAX_FILEFIND_NAME)
	{
		// assert (0);
		return 0;
	}
	memset (lpFindFileData, 0, sizeof(XMFILEFIND));

	//printf ("FS_FindFirstFile %s\n", lpFileName);
	if (FS_FindFirstFile(&lpFindFileData->FindData, lpFileName, lpFileNameBuffer, cbFileNameBuffer) != 0)
	{
		//printf ("FS_FindFirstFile (%s) failed\n", lpFileName);
		return 0;
	}

	lpFindFileData->magic = WIN32_FILEFIND_MAGIC;

	//printf ("GetFileInfo\n");
	GetFileInfo (lpFindFileData);

	//printf ("XM_FindFirstFile End \n");
	return 1;
}

// ������һ���ļ�
XMBOOL 	XM_FindNextFile	(XMFILEFIND * lpFindFileData)
{
	if(lpFindFileData == NULL)
		return 0;
	if(lpFindFileData->magic != WIN32_FILEFIND_MAGIC)
		return 0;

	if(FS_FindNextFile (&lpFindFileData->FindData) == 0)
		return 0;

	GetFileInfo (lpFindFileData);

	return 1;
}

// �ر��ļ�����
VOID XM_FindClose		(XMFILEFIND * lpFindFileData)
{
	if(lpFindFileData == NULL)
		return;
	if(lpFindFileData->magic != WIN32_FILEFIND_MAGIC)
		return;
	lpFindFileData->magic = 0;
	FS_FindClose (&lpFindFileData->FindData);
}

// ��ȡ�ļ��Ĵ�С
DWORD XM_GetFileSize (const char *pFileName)
{
	FS_FILE *pFile;
	DWORD size;

	pFile = FS_FOpen (pFileName, "rb");
	if(pFile == NULL)
		return (DWORD)(-1);
	size = FS_GetFileSize (pFile);
	FS_FClose(pFile);
	return size;
}

// ɾ���ļ�
XMBOOL	XM_RemoveFile (const char *pFileName)
{
	// ����ⲿACC��Դ״��
	if(xm_power_check_acc_safe_or_not() == 0)
	{
		// �ǰ�ȫ��ѹ
		return 0;
	}

	if(FS_Remove (pFileName) == 0)
		return 1;
	else
		return 0;
}

// ���ļ���Ŀ¼�ƶ����µ�λ��
// 0   �ƶ��ɹ�
// -1  �ƶ�ʧ��
int XM_MoveFile (const char * pOldName, const char *pNewName)
{
	int ret;
	// ����ⲿACC��Դ״��
	if(xm_power_check_acc_safe_or_not() == 0)
	{
		// �ǰ�ȫ��ѹ
		return -1;
	}

	ret = FS_Move (pOldName, pNewName);
	if(ret == 0)
		return 0;
	else
		return -1;
}

// �����ļ�Ŀ¼
XMBOOL	XM_MkDir (const char *pDirName)
{
	int ret;

	// ����ⲿACC��Դ״��
	if(xm_power_check_acc_safe_or_not() == 0)
	{
		// �ǰ�ȫ��ѹ
		return 0;
	}
	ret = FS_MkDir (pDirName);
	//printf ("FS_MkDir %s ret %d\n", pDirName, ret);
	if(ret == 0)
		return 1;
	else
		return 0;
}

// ɾ���ļ�Ŀ¼
XMBOOL	XM_RmDir (const char *pDirName)
{
	// ����ⲿACC��Դ״��
	if(xm_power_check_acc_safe_or_not() == 0)
	{
		// �ǰ�ȫ��ѹ
		return 0;
	}
	if(FS_RmDir (pDirName) == 0)
		return 1;
	else
		return 0;
}

XMBOOL XM_IsVolumeMounted (int dev)
{
	// ����ļ�ϵͳ�Ƿ����
	if(FS_IsVolumeMounted ("mmc:0:") == 0)
		return 0;
	else
		return 1;
}

XMBOOL	XM_GetDiskFreeSpace (
  const char * lpRootPathName,          // root path
  DWORD * lpSectorsPerCluster,     // sectors per cluster
  DWORD * lpBytesPerSector,        // bytes per sector
  DWORD * lpNumberOfFreeClusters,  // free clusters
  DWORD * lpTotalNumberOfClusters,  // total clusters
  const char *volpath
)
{
	FS_DISK_INFO Info;
	//int status;

	// ����ļ�ϵͳ�Ƿ����
	if(FS_IsVolumeMounted (volpath) == 0)
		return 0;

	/*
	status = FS_GetVolumeStatus("mmc:0:");
	if(status != FS_MEDIA_IS_PRESENT)
	{
		printf ("XM_GetDiskFreeSpace Error, No Volume\n");
		return 0;
	}
	*/

	if(FS_GetVolumeInfo (volpath, &Info) == 0)
	{
		*lpSectorsPerCluster = Info.SectorsPerCluster;
		*lpBytesPerSector = Info.BytesPerSector;
		*lpNumberOfFreeClusters = Info.NumFreeClusters;
		*lpTotalNumberOfClusters = Info.NumTotalClusters;
		return 1;
	}
	else
	{
		return 0;
	}
}



// �ļ����Զ�ȡ
DWORD XM_GetFileAttributes (
  const char * lpFileName   // name of file or directory
)
{
	U8 Attributes;
	Attributes = FS_GetFileAttributes (lpFileName);
	if(Attributes == 0xFF)
		return (DWORD)(-1);
	return (DWORD)Attributes;
}

// �ļ���������
XMBOOL	XM_SetFileAttributes (
  const char * lpFileName,      // file name
  DWORD dwFileAttributes   // attributes
)
{
	int ret = FS_SetFileAttributes (lpFileName, (U8)dwFileAttributes);
	if(ret == 0)	// == 0: The attributes have been successfully set.
		return 1;
	else				// != 0: In case of any error.
		return 0;
}

// ��ȡ�ļ��Ĵ���ʱ��
// -1		��ȡʧ��
//	0		��ȡ�ļ�����ʱ��ɹ�
int XM_GetFileCreateTime (const char *pFileName, XMSYSTEMTIME * pSystemTime)
{
	int ret;
	U32 TimeStamp;
	FS_FILETIME FileTime;
	if(pFileName == NULL || *pFileName == '\0')
		return -1;
	if(pSystemTime == NULL)
		return -1;
	ret = FS_GetFileTime (pFileName, &TimeStamp);
	if(ret != 0)
		return -1;
	FS_TimeStampToFileTime(TimeStamp, &FileTime);
	pSystemTime->wYear = FileTime.Year;
	pSystemTime->bMonth = FileTime.Month;
	pSystemTime->bDay = FileTime.Day;
	pSystemTime->bHour = FileTime.Hour;
	pSystemTime->bMinute = FileTime.Minute;
	pSystemTime->bSecond = FileTime.Second;
	return 0;
}

int XM_FlagWaterMarkPngImageLoad (char *lpFlagBuffer, int cbFlagBuffer)
{
	return -1;
}

int XM_FlagWaterMarkPngImageSave (char *lpFlagBuffer, int cbFlagBuffer)
{
	return -1;
}


/*
void	speech_init		(char *stream_buff, unsigned int stream_buff_size)
{
}
int	speech_decode	(char **bit_stream, short *pcm_stream, unsigned long compose_type)
{
	return -1;
}
*/

#include "xm_kernel_err_code.h"
void XM_TraceKernelErrCode (int nErrCode)
{
	printf ("kernel Exception, Code = (%d)\n", nErrCode);
}

// �ļ��б����
void	XMSYS_MessageSocketFileListUpdate (unsigned char channel,
													  unsigned char type,
													  unsigned char file_name[8],
													  unsigned int create_time,		// �ļ�����ʱ��
													  unsigned int record_time,		// ��Ƶ¼��ʱ��(��)
													  unsigned short code)
{
	// 20170702
	// ÿ����Ƶ�����ʱ, ö���������ݿ����Ƶ���Ƚ����Ѵ���ʱ��, ռ�ÿ�����ʱ��.
	// ��Ƶ�ļ�Խ��, ����ʱ��Խ��.
	// ��SD�������쳣ʱ, �˲�������������ϵͳ�������̱߳�����
	// ȡ������������Ƶ�б���²���, ���������˷���ʱ�Ŷ�ȡ�����¸��б�
	//
	// �Ż�¼��ʱ���ȡ����
	PostMediaFileListMessage (channel, type);
}

void XMSYS_H264CodecSetupFrameMask (unsigned int mask)
{
}


XMBOOL	XM_PostVideoItemEvent (UINT VideoItemEvent,
										  VOID *lpVideoItemEventParam,
										  UINT cbVideoItemEventParam)
{
	return 1;
}

static int is_valid_address (void *address)
{
	unsigned int base = (unsigned int)address;
	if(base >= 0x80000000 && base < 0x84000000)
		return 1;
	if(base >= 0x90000000 && base < 0x94000000)
		return 1;
	//printf ("invalid address 0x%08x\n", base);
	return 0;
}

#define	DBG_EXT_SIZE		32

int kernel_mcheck (void* pMemBlock)
{
	unsigned int n;
	unsigned int size_aligned;
	unsigned char *data;
	unsigned char *base;
	unsigned int vaddr;
	if(pMemBlock == NULL)
		return 0;
	if(pMemBlock)
	{
		data = (unsigned char *)pMemBlock;
		if((unsigned int)data & 0x1F)		// 32�ֽڶ���
		{
			printf ("kernel_mcheck ng, addr=0x%08x un-aligned\n", (unsigned int)data);
			return -1;
		}
		if(!is_valid_address(data))
		{
			printf ("kernel_mcheck ng, addr=0x%08x invalid\n", (unsigned int)data);
			return -1;
		}

		base = (unsigned char *)(*(unsigned int *)(((unsigned char *)pMemBlock) - 4));
		if(!is_valid_address(base))
		{
			printf ("kernel_mcheck ng, addr=0x%08x, base=0x%08x invalid\n", (unsigned int)data, (unsigned int)base);
			return -1;
		}
		vaddr = *(unsigned int *)(base + 0);
		if(vaddr != (unsigned int)pMemBlock)
		{
			printf ("kernel_mcheck ng, addr=0x%08x, vaddr(0x%08x) mismatch\n", (unsigned int)pMemBlock, (unsigned int)vaddr);
			return -1;
		}

		n = *(unsigned int *)(base + 4);
		size_aligned = (n + 0x1F) & (~0x1F);
		vaddr = (unsigned int)(data + size_aligned + DBG_EXT_SIZE);
		if(!is_valid_address((void *)vaddr))
		{
			printf ("kernel_mcheck ng, addr=0x%08x, baddr(0x%08x) invalid\n", (unsigned int)pMemBlock, (unsigned int)vaddr);
			return -1;
		}
		if(*(unsigned int *)(vaddr) != (unsigned int)pMemBlock)
		{
			printf ("kernel_mcheck ng, addr=0x%08x, n=0x%08x, tail=0x%08x\n", pMemBlock, n,  *(unsigned int *)(vaddr));
			return -1;
		}

	}
	return 0;
}

#if defined(CORE_HEAP_DEBUG)
void* _kernel_malloc(unsigned int n, char *file, int line)
#else
void* kernel_malloc(unsigned int n)
#endif
{
	unsigned char *base;
	//n = (n + 64 + 0x3F) & (~0x3F);
	// 16�ֽ� = ID(data, 4�ֽ�) + ����(n, 4�ֽ�) + DUMMY( >= 0) + BASE(base, 4�ֽ�) + DATA(n) + ID(data, 4�ֽ�)
	// DUMMYȡ���ڿ�϶�ĳ���
	if(n == 0)
		return NULL;

	// 32�ֽڶ���
	unsigned int data_len_aligned = (n + 0x1F) & (~0x1F);

	unsigned int size = data_len_aligned + 64 + DBG_EXT_SIZE;		// DBG_EXT_SIZE��ŵ���debug��Ϣ

	base = (unsigned char *)OS_malloc (size);
	if(base)
	{
		unsigned char *data = (unsigned char *)(((unsigned int)base + 0x1F) & (~0x1F));
		if(data - base < 12)	// ID(4) + ����(4) + dummy(0) + base(4)
			data += 32;
		*(unsigned int *)(base + 0) = (unsigned int)(data);		// ID(data, 4�ֽ�)
		*(unsigned int *)(base + 4) = n;		// ����(n, 4�ֽ�)
		*(unsigned int *)(data - 4) = (unsigned int)(base);		// BASE(base, 4�ֽ�)
		*(unsigned int *)(data + data_len_aligned + DBG_EXT_SIZE) = (unsigned int)(data);

		if(DBG_EXT_SIZE)
		{
			memset (data + data_len_aligned, 0x00, DBG_EXT_SIZE);
		}
		//printf ("kernel_malloc base = 0x%08x, n = 0x%08x\n", (unsigned int)data, (unsigned int)n);
#if defined(CORE_HEAP_DEBUG)
	OS_EnterRegion ();
	printf ("h_malloc  0x%08x size 0x%08x file=%s, line=%d\n", (unsigned int)(data), n, file, line);
	//printf ("h_malloc  0x%08x size 0x%08x\n", (unsigned int)(data), n);
	OS_LeaveRegion ();
#endif

		return (void *)data;
	}
	else
	{
		printf ("kernel_malloc failed, size=0x%08x\n", n);
	}
	return 0;

	//return OS_malloc (n);
}

#if defined(CORE_HEAP_DEBUG)
void* _kernel_mallocz (unsigned int n, char *file, int line)
#else
void* kernel_mallocz (unsigned int n)
#endif
{
#if defined(CORE_HEAP_DEBUG)
	void *mem = _kernel_malloc (n, file, line);
#else
	void *mem = kernel_malloc (n);
#endif
	if(mem)
		memset (mem, 0, n);

	return mem;
}

#if defined(CORE_HEAP_DEBUG)
void  _kernel_free  (void* pMemBlock, char *file, int line)
#else
void  kernel_free  (void* pMemBlock)
#endif
{
	unsigned int n;
	unsigned int size_aligned;
	unsigned int vaddr;
	unsigned char *data;
	unsigned char *base;
	if(pMemBlock)
	{
		//printf ("kernel_free base = 0x%08x\n", (unsigned int)pMemBlock);
		data = (unsigned char *)pMemBlock;
		if(!is_valid_address(data))
		{
			printf ("kernel_free ng, illegal addr=0x%08x\n", (unsigned int)data);
			return;
		}
		if((unsigned int)data & 0x1F)		// 32�ֽڶ���
		{
			printf ("kernel_free ng, addr=0x%08x un-align\n", (unsigned int)data);
			return;
		}
		base = (unsigned char *)(*(unsigned int *)(((unsigned char *)pMemBlock) - 4));
		if(!is_valid_address(base))
		{
			printf ("kernel_free (0x%08x) ng, illegal base=0x%08x\n", (unsigned int)pMemBlock, (unsigned int)base);
			return;
		}
		vaddr = *(unsigned int *)(base + 0);
		if(vaddr != (unsigned int)pMemBlock)
		{
			printf ("kernel_free (0x%08x) ng, vaddr(0x%08x) error\n", (unsigned int)pMemBlock, (unsigned int)vaddr);
			return;
		}

		n = *(unsigned int *)(base + 4);
		size_aligned = (n + 0x1F) & (~0x1F);
		vaddr = (unsigned int)(data + size_aligned + DBG_EXT_SIZE);
		if(!is_valid_address((void *)vaddr))
		{
			printf ("kernel_free (0x%08x) ng, vaddr(0x%08x) illegal\n", (unsigned int)pMemBlock, (unsigned int)vaddr);
			return;
		}
		if(*(unsigned int *)(vaddr) != (unsigned int)pMemBlock)
		{
			printf ("kernel_free (0x%08x) ng, vaddr(0x%08x)'s data(0x%08x) error\n", (unsigned int)pMemBlock, (unsigned int)vaddr, *(unsigned int *)(vaddr));
			return;
		}

		*(unsigned int *)(base + 0) = 0;
		*(unsigned int *)(base + 4) = 0;
		*(unsigned int *)(data + size_aligned + DBG_EXT_SIZE) = 0;
		if(DBG_EXT_SIZE)
		{
			for (int i = 0; i < DBG_EXT_SIZE; i ++)
			{
				if( *(data + size_aligned + i) != 0)
				{
					printf ("kernel_free (0x%08x) ng, broken rev-area (0x%08x)\n", (unsigned int)pMemBlock, data + size_aligned);
					break;
				}
			}
			memset (data + size_aligned, 0xFF, DBG_EXT_SIZE);
		}

#if defined(CORE_HEAP_DEBUG)
		OS_EnterRegion ();
		printf ("h_free 0x%08x file=%s, line=%d\n", (unsigned int)(pMemBlock), file, line);
		OS_LeaveRegion ();
#endif

		OS_free (base);
	}
	//OS_free (pMemBlock);
}

#if defined(CORE_HEAP_DEBUG)
void* _kernel_calloc (unsigned int n, unsigned int s, char *file, int line)
#else
void* kernel_calloc (unsigned int n, unsigned int s)
#endif
{
#if defined(CORE_HEAP_DEBUG)
	void *mem = _kernel_malloc (n * s, file, line);
#else
	void *mem = kernel_malloc (n * s);
#endif
	if(mem)
	{
		memset (mem, 0, n * s);
	}
	return mem;
}

#if defined(CORE_HEAP_DEBUG)
void *_kernel_realloc(void *ptr, unsigned int size, char *file, int line)
#else
void *kernel_realloc(void *ptr, unsigned int size)
#endif
{
	//return OS_realloc (ptr,size);
	char *new_ptr;
	unsigned int n;
	unsigned char *data;
	unsigned char *base;
	if(ptr == NULL)
	{
#if defined(CORE_HEAP_DEBUG)
		return _kernel_malloc(size, file, line);
#else
		return kernel_malloc(size);
#endif
	}
	if(size == 0)
	{
#if defined(CORE_HEAP_DEBUG)
		_kernel_free (ptr, file, line);
#else
		kernel_free (ptr);
#endif
		return NULL;
	}

	data = (unsigned char *)ptr;
	if((unsigned int)data & 0x1F)		// 32�ֽڶ���
	{
		printf ("kernel_realloc(0x%08x) ng, un-align\n", (unsigned int)data);
		return NULL;
	}
	if(!is_valid_address(data))
	{
		printf ("kernel_realloc(0x%08x) ng\n", (unsigned int)data);
		return NULL;
	}
	base = (unsigned char *)(*(unsigned int *)(data - 4));
	if(!is_valid_address(base))
	{
		printf ("kernel_realloc(0x%08x) ng, base(0x%08x) invalid\n", (unsigned int)data, (unsigned int)base);
		return NULL;
	}
	if( *(unsigned int *)base != (unsigned int)ptr )
	{
		printf ("kernel_realloc(0x%08x) ng, base(0x%08x)'s data(%08x) mis-match\n", ptr, base, *(unsigned int *)base);
		return NULL;
	}
	n = *(unsigned int *)(base + 4);
	if(size < n)
	{
		printf ("kernel_realloc warn, addr=0x%08x, size=0x%08x, n=0x%08x\n", ptr, size, n);
	}
#if defined(CORE_HEAP_DEBUG)
	new_ptr = _kernel_malloc (size, file, line);
#else
	new_ptr = kernel_malloc (size);
#endif
	if(new_ptr)
	{
		unsigned int n1 = n;
		if(n1 > size)
			n1 = size;
		memcpy (new_ptr, ptr, n1);

#if defined(CORE_HEAP_DEBUG)
		_kernel_free (ptr, file, line);
#else
		kernel_free (ptr);
#endif
	}

	return (void *)new_ptr;
}

extern INT32 IsCardExist(UINT32 ulCardID);

// �������SD���Ƿ����
int XMSYS_check_sd_card_exist (void)
{
	if(IsCardExist(SDMMC_DEV_0) == 1)
		return 1;
	else
		return 0;
}

extern void dma_memcpy (unsigned char *dst_buf, unsigned char *src_buf, unsigned int len);

void *fast_memcpy (void * s1, const void * s2, size_t n)
{
	if( ((unsigned int)s1 & 1) || ((unsigned int)s2 & 1) )
	{
		return memcpy (s1, s2, n);
	}

	if(n & 1)
	{
		n --;
		dma_memcpy ((unsigned char *)s1, (unsigned char *)s2, n);
		*(((char *)s1) + n) = *(((char *)s2) + n);
	}
	else
	{
		dma_memcpy ((unsigned char *)s1, (unsigned char *)s2, n);
	}

	//return memcpy (s1, s2,n);
	return s1;
}
