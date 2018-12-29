//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	SongXing
//
//	File name: desktop.c
//	  �г���¼״̬������
//	  ������һ�����ص�ϵͳ���ƴ��ڣ������¼���ȱʡ������֧��ʵ����ʾ
//
//	Revision history
//
//		2012.09.01	SongXing Initial version
//
//****************************************************************************
/*
*/
#include <xm_proj_define.h>
#include "rtos.h"
#include "app.h"
#include "app_voice.h"
#include "app_menuid.h"
#include "system_check.h"
#include "systemapi.h"
#include "xm_videoitem.h"
#include <xm_h264_codec.h>
#include "rom.h"
#include "xm_uvc_socket.h"
#include "xm_systemupdate.h"
#include "xm_core.h"
#include "xm_video_task.h"
#include "types.h"

#include "lcd.h"
#include "rxchip.h"

#include "trigger_array_stack.h"
#include "xm_app_menudata.h"

extern int ITU656_in;
extern int Reversing;
extern void dettask_send_mail(char ret);

int ShowLogo (UINT32 LogoAddr,UINT32 LogoSize);
unsigned int XMSYS_system_update_get_version (void);
// �������SD���Ƿ����
extern int XMSYS_check_sd_card_exist (void);
int XMSYS_JpegCodecOneKeyPhotograph (void);

int PowerOnShowLogo=0;
unsigned int PhtographStart=0;
unsigned int DispDesktopMenu=1;
int Red_Location = 0;

u8_t LCD_Rotate_Data = 0;

extern u8_t ACC_Det_Start;
BOOL ACC_DET_First = TRUE;
u8_t AHD_ChannelNum_Data = 1;
u8_t Manual_Channel = 1;
extern u8_t ACC_Select;
BOOL Data_Select_AV_AND = FALSE;
extern u8_t ACC_Det_Channel;
BOOL ACC_Select_BiaoCHi_Show = FALSE;
extern BOOL Close_Audio_Sound;
BOOL Close_Reverse_Back = FALSE;

extern u8_t pipmode;

// �����Լ�
static VOID DesktopOnSystemCheck (void);
static void OpenSystemUpdateDialog (void);	// ��ϵͳ������ʾ�Ի���

#define	OSD_AUTO_HIDE_TIMEOUT			10000

#define DEMO_MODE			0			// ��ʾģʽ(�޿����롢��д��������)
#define WAITING_MODE		1			// �ȴ�ģʽ(��ʱ��¼�ȴ�)
#define RECORD_MODE		2			// ��¼ģʽ

// ϵͳ�����Լ���̶���
enum {
	SELFCHECK_STEP_BATTERY = 0,	// ���״�����
	SELFCHECK_STEP_TIME,				// ϵͳʱ�����ü��
	SELFCHECK_STEP_CARD,				// �����ļ�ϵͳ���
	SELFCHECK_STEP_VIDEO,			// ��Ƶ���¼��ؼ��
	SELFCHECK_STEP_BYPASS,			// ϵͳ�����Լ������
	SELFCHECK_STEP_COUNT
};


/*
ϵͳԤ������, �״��������ط�ģʽΪ��������ͷ, ȱʡΪ��¼ģʽ
*/

int	iPlaybackMode;		// ����ʾ����ʾ��һЩ����ͷ�Ļ���
int	iRecordMode;		// ��ǰ���г���¼��ʽ:  Demo ��Record��WAITING
int CurrentPhotoMode =0;
static int SelfCheckStep = SELFCHECK_STEP_BATTERY;
static unsigned int ticket_to_card_verify_warning;

#define	XM_USER_SYSCHECKING	(XM_USER+0)
#define BATTERY_LOW_POWER_OFF_TIMEOUT   (5 * 1000)

// ��ǿ����¼��
// �����·��ص�����ģʽʱ�����¼��
void APPMarkCardChecking (int stop_record)
{
	if(stop_record)
	{
		if(StopRecord () < 0)
		{
			XM_printf ("StopRecord NG\n");
			return;
		}
		else
		{
			XM_printf ("%d,StopRecord OK\n",XM_GetTickCount());
		}
	}

	SelfCheckStep = SELFCHECK_STEP_CARD;
	iRecordMode = RECORD_MODE;

	// ���ó�ʼICON״̬
	XM_SetFmlDeviceCap (DEVCAP_VIDEO_REC, DEVCAP_VIDEO_REC_STOP);
}

#include "gpio.h"
#include "ark1960.h"
u8_t Auto_Switch_3s = 0;
extern BOOL Close_Brighness;
//extern BOOL NO_Signed_Fail;
BOOL Auto_Menu = FALSE; //���뵹��״̬,����Desktop����,Ȼ�󰴰������ǵ��з�Ӧ
u8_t Data_PowerOff_Count = 0;

VOID DesktopOnEnter (XMMSG *msg)
{
    CurrentPhotoMode = 0;
    //NO_Signed_Fail = TRUE;
	if(SelfCheckStep == SELFCHECK_STEP_BATTERY)
	{
		// ������λ
		// ���ó�ʼICON״̬
		if(ACC_DET_First) //��ֹ������������¼��״̬
		{
		    XM_SetFmlDeviceCap (DEVCAP_VIDEO_REC, DEVCAP_VIDEO_REC_STOP);
        }
	}
    AP_SetMenuItem(APPMENUITEM_LCD, AP_GetMenuItem(APPMENUITEM_LCD));
    #if 0
	if (msg->wp == 0)
	{
		// ���洴��
		DesktopOnSystemCheck ();
	}
	else
	{
		// �������½���
		DesktopOnSystemCheck ();
	}
    #endif
//
	//������ҳ��Դ������
	#if 1
	//XM_RomImageOpen(ROM_T18_COMMON_MENU_MENU_BG_PNG, ROM_T18_COMMON_MENU_MENU_BG_PNG_SIZE);
	//XM_RomImageOpen(ROM_T18_COMMON_DESKTOP_BIAOCHI_PNG,ROM_T18_COMMON_DESKTOP_BIAOCHI_PNG_SIZE);
	//XM_RomImageOpen(ROM_T18_COMMON_MENU_MENU_TOP_PNG, ROM_T18_COMMON_MENU_MENU_TOP_PNG_SIZE);
	//XM_RomImageOpen(ROM_T18_COMMON_MENU_MENU_BOTTOM_BAR_PNG, ROM_T18_COMMON_MENU_MENU_BOTTOM_BAR_PNG_SIZE);
	//XM_RomImageOpen(ROM_T18_COMMON_MENU_MENU_BOTTOM_BAR_A_PNG, ROM_T18_COMMON_MENU_MENU_BOTTOM_BAR_A_PNG_SIZE);

	//XM_RomImageOpen(ROM_T18_COMMON_MENU_ICON_DATE_PNG, ROM_T18_COMMON_MENU_ICON_DATE_PNG_SIZE);
	//XM_RomImageOpen(ROM_T18_COMMON_MENU_ICON_MOT_PNG, ROM_T18_COMMON_MENU_ICON_MOT_PNG_SIZE);
	//XM_RomImageOpen(ROM_T18_COMMON_MENU_ICON_DEC_PNG, ROM_T18_COMMON_MENU_ICON_DEC_PNG_SIZE);
	//XM_RomImageOpen(ROM_T18_COMMON_MENU_ICON_COLOR_PNG, ROM_T18_COMMON_MENU_ICON_COLOR_PNG_SIZE);
	//XM_RomImageOpen(ROM_T18_COMMON_MENU_ICON_VIDEO_PNG, ROM_T18_COMMON_MENU_ICON_VIDEO_PNG_SIZE);
	//XM_RomImageOpen(ROM_T18_COMMON_MENU_ICON_SYS_PNG, ROM_T18_COMMON_MENU_ICON_SYS_PNG_SIZE);
	#endif
    if(PowerOnShowLogo == 0)
    {
        //APP_SetPowerOff_Restore(1);//�Զ��ػ���,��Ϊ�ػ�ģʽ
		XM_printf(">>>>>>APP_GetMemory():%d\r\n", APP_GetMemory());
		XM_printf(">>>>>>AP_GetLogo():%d\r\n", AP_GetLogo());
		
        //if(APP_GetMemory()) { //�ػ�ģʽ
        //    XM_KeyEventProc (VK_AP_POWER,XM_KEYDOWN); //�Զ��л�
        //}
		//else
		{
            if(AP_GetLogo())
            {
    		    ShowLogo(ROM_T18_COMMON_DESKTOP_LOGO_POWERON_JPG,ROM_T18_COMMON_DESKTOP_LOGO_POWERON_JPG_SIZE);
                LCD_set_enable (1);
                OS_Delay (1000);
            }
			//APP_SetPowerOff_Restore(0);
    		PowerOnShowLogo=1;
            Red_Location = AP_GetRed_Location();
            XMSYS_VideoOpen ();
    		XM_printf("show logo end\n");
        	DispDesktopMenu=1;
            //��������,��������.
            #if 0
            if(AP_GetBright_Switch() == 0){
				APP_GetPowerOn_Memory(0); //����Ϊ�ػ�״̬.��Ҫ���浱ǰ״̬
			}else {
            	APP_SetPowerOn_Memory(Close_Brighness);//���õ�ǰ�����ģʽ
			}
			#endif
			APP_SetPowerOn_Memory(Close_Brighness);//���õ�ǰ�����ģʽ
        }

		APP_SetPowerOff_Restore(1);
		APP_SaveMenuData ();
    }
    XM_voice_prompts_insert_voice (XM_VOICE_ID_STARTUP);
    Auto_Switch_3s = 0;// ��ʼ��3s�ֻ�����
    Auto_Menu = FALSE;
    Data_PowerOff_Count = 0; //���ؽ��濪ʼ��ʱ
    XM_SetTimer (XMTIMER_DESKTOPVIEW, 500); // 500
}

// �����Լ�
static VOID DesktopOnSystemCheck (void)
{
	DWORD	dwState;

	// ����Ƿ����Զ������汾(�ڲ�����ʹ��)��Ҫ��װ, ������, �����ʱ������
	if(XMSYS_system_update_get_version() == 0xFFFFFFFF)
	{
		//AP_OpenSystemUpdateView (1);
		return;
	}
	if(XMSYS_system_update_busy_state())
	{
		return;
	}

	if(SelfCheckStep == SELFCHECK_STEP_BATTERY)
	{
		// ��ؼ��״̬
		// ��ؼ��OK, �л���RTCʱ����״̬
		SelfCheckStep = SELFCHECK_STEP_TIME;
	}

	if(SelfCheckStep == SELFCHECK_STEP_TIME)
	{
		// ���ʱ������
		dwState = XM_GetFmlDeviceCap(DEVCAP_TIMESETTING);
		if(dwState == DEVCAP_TIMESETTING_NG)
		{
			// ����һ��ȱʡʱ��, �����޷���ʼ��¼��Ƶ.
			// ��Ƶ������ҪRTCʱ��������
			XMSYSTEMTIME local_time;
			local_time.wYear = 2017;
			local_time.bMonth = 1;
			local_time.bDay = 1;
			local_time.bHour = 0;
			local_time.bMinute = 0;
			local_time.bSecond = 0;
			local_time.wMilliSecond = 0;
			XM_SetLocalTime (&local_time);

			// δ���ã������ʼ���ý���
			//XM_PushWindow (XMHWND_HANDLE(InitSettingView));
			//return;
		}

		// ʱ����PASS, �л��������
		SelfCheckStep = SELFCHECK_STEP_CARD;
	}

	if(SelfCheckStep == SELFCHECK_STEP_CARD)
	{
		// �����ģʽ. ��鿨�Ƿ���ȷ�Ĳ���
		if(XM_GetFmlDeviceCap(DEVCAP_SDCARDSTATE) != DEVCAP_SDCARDSTATE_INSERT)
		{
			// ��û����ȷ�Ĳ���, �л�����ʾģʽ
			goto demo_mode;
		}

		// ������ȷ����, �л���¼����ģʽ
		SelfCheckStep = SELFCHECK_STEP_VIDEO;
	}

	if(SelfCheckStep == SELFCHECK_STEP_VIDEO)
	{
		// ¼����ģʽ, ���ϵͳ�Ƿ����������Ƶ¼�������
		// �����Ƶ����������Ƿ��ѿ���
		if(!XM_VideoItemIsBasicServiceOpened())
		{
			// ��Ƶ����������ʧ��
			// �л�����ʾģʽ
			goto demo_mode;
		}
		SelfCheckStep = SELFCHECK_STEP_BYPASS;
	}

	if(SelfCheckStep != SELFCHECK_STEP_BYPASS)
		return;

#if CZS_USB_01
	// ����¼��
#else
	// �����ʱ¼���Ƿ�ʹ��
	if((XM_GetTickCount() - AppGetStartupTicket()) < (AP_GetRecordDelayTime () * 1000))
	{
		// ¼����ʱ
		XM_SetTimer (XMTIMER_DESKTOPVIEW, 100);	// ����100ms�Ķ�ʱ��
		iRecordMode = WAITING_MODE;
		return;
	}
	else
#endif
	{
		if(StartRecord () == 0)
		{
			XM_KillTimer (XMTIMER_DESKTOPVIEW);
			iRecordMode = RECORD_MODE;
			printf ("SystemCheck REC OK\n");
		}
		else
		{
			printf ("SystemCheck REC NG\n");
			// ������ʱ����100ms�����������¼ģʽ
			XM_SetTimer (XMTIMER_DESKTOPVIEW, 100);
			iRecordMode = WAITING_MODE;
		}
	}

	//return;
	// ��ʾģʽ
demo_mode:
	XM_KillTimer (XMTIMER_DESKTOPVIEW);
	iRecordMode = DEMO_MODE;
	XMSYS_H264CodecSetForcedNonRecordingMode(1); //��SD���ϵ��޷���"ֹͣ¼������"�½������ò˵�
	XM_SetFmlDeviceCap (DEVCAP_VIDEO_REC, DEVCAP_VIDEO_REC_STOP);
	XM_printf(">>>>>DesktopOnSystemCheck start recorder..........\r\n");

	XMSYS_H264CodecRecorderStart();	//  ����¼��ͨ��
}


/*
OnLeave
	�����������棬��Ҫ����UI����ʱϵͳ�����ݻط��Ƿ�ֹͣ�������Ƿ񽫻ط���UI�ϳ�
*/
VOID DesktopOnLeave (XMMSG *msg)
{
	// ɾ����ʱ��
	XM_KillTimer (XMTIMER_DESKTOPVIEW);
	if (msg->wp == 0)
	{
		XM_printf ("Desktop Exit\n");
	}
	else
	{
		XM_printf ("Desktop Push\n");
	}
}

/*
OnTimer����ʱ������������״̬��˸

	�����ʱ��¼��ʱ����
		�ر�UI
		��ʼ��¼�û���װ����������ͷ���档�������жϵ�����ͷ�Թ̶�ģʽ�Ļ����¼��

	����ϵͳ���ã�����ͼ����Ҫռ����������ñ�����Ӧ����ʾ�ռ䣬����״̬ͼ�갴�̶�Ƶ����˸��������ͷ����״̬�����״̬��������λͼ����ʾ������λ��

	�����Demoģʽ�����̶�Ƶ�ʿ���UI����ʾ��ʾ״̬�������û�
*/

BOOL Guide_Camera_Show(VOID) 
{
#if 0
    if(AHD_Guide_Start() &&((AP_GetGuide_Camera1() &&(ACC_Det_Start == 1)) || (AP_GetGuide_Camera2() &&(ACC_Det_Start == 2)) \
       || (AP_GetGuide_Camera3() &&(ACC_Det_Start == 3)) || (AP_GetGuide_Camera4() &&(ACC_Det_Start == 4))))
    {
        return TRUE;
    }
    return FALSE;
#endif
   //if(AHD_Guide_Start() &&((AP_GetGuide_Camera1() &&(pipmode == 1)) || (AP_GetGuide_Camera2() &&(pipmode == 2))))
   //XM_printf("---------------> get_parking_trig_status : %d\n",get_parking_trig_status());
   //XM_printf("---------------> AP_GetMenuItem : %d\n",AP_GetMenuItem(APPMENUITEM_PARKING_LINE));
   if( (get_parking_trig_status() != 0) && AP_GetMenuItem(APPMENUITEM_PARKING_LINE))
   {
       return TRUE;
   }
   return FALSE;
}

void deskTopStartRecord()
{
    if( (XM_GetFmlDeviceCap(DEVCAP_SDCARDSTATE) == DEVCAP_SDCARDSTATE_INSERT) )
	{		
		if( (rxchip_get_check_flag()==TRUE) )
		{
			rxchip_set_check_flag(FALSE);
			if(StartRecord() == 0)
			{
				//XM_KillTimer (XMTIMER_DESKTOPVIEW);
				iRecordMode = RECORD_MODE;
				//XM_printf ("OnTimer REC OK\n");
			}
			else
			{
				//XM_printf ("OnTimer REC NG\n");
			}
		}
	}
}

static BOOL is_parking_back = FALSE;
VOID DesktopOnTimer (XMMSG *msg)
{
	//XM_printf(">>>>>>>>DesktopOnTimer, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);
    //����������־
    if((Guide_Camera_Show() ||ACC_Select_BiaoCHi_Show)&& ACC_DET_First&& Close_Audio_Sound)
    {
        ACC_DET_First = FALSE;
        XM_PushWindow (XMHWND_HANDLE(CarBackLineView));
    }else{
        ACC_DET_First = TRUE;
    }

	
    //3s �Զ�ѭ���л����� ���رձ����ʱ��,�����л�
    if(APP_GetAuto_Switch() && !Close_Brighness&&Close_Audio_Sound)
    {
        Auto_Switch_3s ++;
        if(Auto_Switch_3s > 12)
        {
            Auto_Switch_3s = 0;
            XM_PostMessage (XM_KEYDOWN, VK_AP_AV, 0); //�Զ��л�
        }
    }

    Data_PowerOff_Count ++;
	
    if(get_parking_trig_status())
    {
        is_parking_back = TRUE;
        if(AP_GetMenuItem(APPMENUITEM_POWER_STATE) == POWER_OFF)
        {
            HW_LCD_BackLightOn();
        }
    }
    else if(is_parking_back)
    {
        is_parking_back = FALSE;
        if(AP_GetMenuItem(APPMENUITEM_POWER_STATE) == POWER_OFF)
        {
            HW_LCD_BackLightOff();
        }
    }
	#if 0
	if((Close_Audio_Sound == FALSE) && get_parking_trig_status()) {
        Close_Reverse_Back = TRUE;
		//PowerOn_PowerKey();
        Data_PowerOff_Count = 0;
	}else if((Close_Reverse_Back == TRUE)&& !AHD_Guide_Start()&&(Data_PowerOff_Count > 5)){ //�������������´��ڹػ�״̬
	    Close_Reverse_Back = FALSE;
        Data_PowerOff_Count = 0;
	    //XM_KeyEventProc (VK_AP_POWER,XM_KEYDOWN);
    }
    #endif

	// 100msΪ��λ
	if(iRecordMode != WAITING_MODE)
	{
		//XM_KillTimer (XMTIMER_DESKTOPVIEW);
	}
	
	if( (XM_GetFmlDeviceCap(DEVCAP_SDCARDSTATE) == DEVCAP_SDCARDSTATE_INSERT) )
	{
		if( (rxchip_get_check_flag()==TRUE) )
		{
			rxchip_set_check_flag(FALSE);
			if(XMSYS_H264CodecGetForcedNonRecordingMode())
			{//��¼��ģʽ
			    // �޷��л���¼��ģʽ��������Ϣ���ڣ���ʾԭ��
				unsigned int card_state = XM_GetFmlDeviceCap(DEVCAP_SDCARDSTATE);
				if(card_state == DEVCAP_SDCARDSTATE_INSERT)
				{
			        XMSYS_H264CodecSetForcedNonRecordingMode(0);//���ó�¼��ģʽ,ǿ�Ʒ�¼��ģʽ��¼��ģʽת��,����¼��
					XM_SetFmlDeviceCap(DEVCAP_VIDEO_REC, DEVCAP_VIDEO_REC_START);
				}
			}

			#if 0
			if(StartRecord() == 0)
			{
				iRecordMode = RECORD_MODE;
			}
			else
			{
			    
			}
			#endif
		}
	}
}


// �ļ�ϵͳ�޷����ʴ���ص�����
static void fs_access_error_alert_cb (void *userPrivate, unsigned int uKeyPressed)
{
	if(uKeyPressed == VK_AP_MENU)		// ����ʽ��������
	{
		// ��ʽ������
		AP_OpenFormatView (0);
	}
	else					// ��ȡ��������
	{
		// ���ص������ߴ���
		XM_PullWindow (0);
	}
}

// �ļ�ϵͳ�����ܻص�����
static void fs_low_performance_alert_cb (void *userPrivate, unsigned int uKeyPressed)
{
	if(uKeyPressed == VK_AP_MENU)		// ����ʽ��������
	{
		// ��ʽ������
		AP_OpenFormatView (0);
	}
	else					// ��ȡ��������
	{
		// ���ص������ߴ���
		XM_PullWindow (0);
	}
}

// �ļ�ϵͳ��дУ�����ص�����
static void fs_verify_error_alert_cb (void *UserPrivate, unsigned int uKeyPressed)
{
	XM_JumpWindowEx (XMHWND_HANDLE(VideoListView), 0, XM_JUMP_POPDEFAULT);
}

VOID DesktopOnKeyRepeatDown (XMMSG *msg)
{
    if(get_trigger_det_status() != 0 || get_delay_return_trig() != TRIGGER_DELAY_END)
    {
        return;
    }
    switch(msg->wp)
    {
        case VK_AP_UP:		// ����¼���
        
            if(AP_GetMidle_RED_Line()) {
                Red_Location = Red_Location + 1;
                AP_SetRed_Location(Red_Location);
                AP_SaveMenuData (&AppMenuData);
            }
            else {
                if(ACC_Select == 4)
                    break;
                if(APP_GetVOl_OnOff())
		            XM_PushWindow (XMHWND_HANDLE(VolView));
            }
			break;

		case VK_AP_DOWN:		// һ������
            if(AP_GetMidle_RED_Line()) {
                Red_Location = Red_Location - 1;
                AP_SetRed_Location(Red_Location);
                AP_SaveMenuData (&AppMenuData);
            }
            else {
                if(ACC_Select == 4) //����ͨ����Ҫ��������
                    break;
                if(APP_GetVOl_OnOff())
		            XM_PushWindow (XMHWND_HANDLE(VolView));
            }
			break;
    }
}

BOOL REMOTE_KEY_UP_ENABLE = FALSE;
BOOL REMOTE_KEY_DOWN_ENABLE = FALSE;

BOOL REMOTE_KEY_MENU_ENABLE = FALSE;
BOOL REMOTE_KEY_LEFT_ENABLE = FALSE;
BOOL REMOTE_KEY_RIGHT_ENABLE = FALSE;
u8_t Backlight_ON_OFF = 0;

VOID CleanRemote_KEY_Enable (VOID)
{
    REMOTE_KEY_LEFT_ENABLE = FALSE;
	REMOTE_KEY_DOWN_ENABLE = FALSE;
    REMOTE_KEY_RIGHT_ENABLE = FALSE;
    REMOTE_KEY_UP_ENABLE = FALSE;
    REMOTE_KEY_MENU_ENABLE = FALSE;
}

BOOL AV_Select_AHD = TRUE;
#if 0
u8_t Struation = 50;
u8_t Brightness  = 50;
u8_t Contrast  = 50;
u8_t sharpness  = 0;
u8_t hue  = 0;
#endif
u8_t sharpness  = 0;
u8_t pipmode = 0;

//int Image_size = 0x0D;
//int Image_Zoom = 2;
VOID DesktopOnKeyDown (XMMSG *msg)
{
// ������
	//XM_Beep (XM_BEEP_KEYBOARD);
    //if(!Close_Audio_Sound)
    //    return ;

    if(get_parking_trig_status() != 0 || get_delay_return_trig() != TRIGGER_DELAY_END)
    {
        return;
    }
	XM_printf(">>>>>>>>DesktopOnKeyDown, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);
	switch(msg->wp)
	{
	    case VK_AP_AV:
            //AP_GetAHD_ChannelNum
           
			if(XMSYS_VideoGetImageAssemblyMode() == XMSYS_ASSEMBLY_MODE_ALL_DISPLAY_Right)
				XMSYS_VideoSetImageAssemblyMode(XMSYS_ASSEMBLY_MODE_ALL_DISPLAY_UP);
			else if(XMSYS_VideoGetImageAssemblyMode() == XMSYS_ASSEMBLY_MODE_ALL_DISPLAY_UP)
				XMSYS_VideoSetImageAssemblyMode(XMSYS_ASSEMBLY_MODE_ALL_DISPLAY_Vert);
			else if(XMSYS_VideoGetImageAssemblyMode() == XMSYS_ASSEMBLY_MODE_ALL_DISPLAY_Vert)
				XMSYS_VideoSetImageAssemblyMode(XMSYS_ASSEMBLY_MODE_ISP601_ONLY);
			else if(XMSYS_VideoGetImageAssemblyMode() == XMSYS_ASSEMBLY_MODE_ISP601_ONLY)
				XMSYS_VideoSetImageAssemblyMode(XMSYS_ASSEMBLY_MODE_ITU656_ONLY);
			else if(XMSYS_VideoGetImageAssemblyMode() == XMSYS_ASSEMBLY_MODE_ITU656_ONLY)
				XMSYS_VideoSetImageAssemblyMode(XMSYS_ASSEMBLY_MODE_USB_ONLY);
			else if(XMSYS_VideoGetImageAssemblyMode() == XMSYS_ASSEMBLY_MODE_USB_ONLY)
				XMSYS_VideoSetImageAssemblyMode(XMSYS_ASSEMBLY_MODE_ISP601_ITU656);
			else if(XMSYS_VideoGetImageAssemblyMode() == XMSYS_ASSEMBLY_MODE_ISP601_ITU656)
				XMSYS_VideoSetImageAssemblyMode(XMSYS_ASSEMBLY_MODE_ALL_DISPLAY_Left);
			else if(XMSYS_VideoGetImageAssemblyMode() == XMSYS_ASSEMBLY_MODE_ALL_DISPLAY_Left)
				XMSYS_VideoSetImageAssemblyMode(XMSYS_ASSEMBLY_MODE_ALL_DISPLAY_Right);
            break;
        case REMOTE_KEY_MENU:
		case VK_AP_MENU:		// �˵���
            if(XM_GetFmlDeviceCap(DEVCAP_VIDEO_REC) == DEVCAP_VIDEO_REC_STOP)
            {
				XM_PushWindow (XMHWND_HANDLE(SettingView));
			}
			
			//AP_OpenFormatView(0);
		    if(AHD_Guide_Start())
				Auto_Menu = TRUE;
			break;

		case VK_AP_MODE:		// �л���¼��ط�ģʽ
			XM_printf(">>>>>>>VK_AP_MODE, iRecordMode:%d\r\n", iRecordMode);
			if(iRecordMode == DEMO_MODE)
			{
				// �޷��л�����ƵԤ��ģʽ��������Ϣ���ڣ���ʾԭ��
				unsigned int card_state = XM_GetFmlDeviceCap(DEVCAP_SDCARDSTATE);
				if(card_state == DEVCAP_SDCARDSTATE_UNPLUG)
				{
					// ���Ѱγ�
					XM_OpenAlertView (
						AP_ID_CARD_MESSAGE_CARD_WITHDRAW,	// ��Ϣ�ı���ԴID
						AP_ID_CARD_ICON_SDCARD,			// ͼƬ��Ϣ��ԴID
						0,
						0,												// ��ť������ԴID
						0,												// ��ť����������ԴID
						APP_ALERT_BKGCOLOR,
						10.0,											// 10�����Զ��ر�
						APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
						NULL,											// �����ص�����
						NULL,											// �û��ص�����˽�в���
						XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
						//XM_VIEW_ALIGN_CENTRE,
						0												// ALERTVIEW��ͼ�Ŀ���ѡ��
						);
				}
				else if(card_state == DEVCAP_SDCARDSTATE_FS_ERROR)
				{
					DWORD dwButtonNormalTextID[2];
					DWORD dwButtonPressedTextID[2];
					dwButtonNormalTextID[0] = AP_ID_ALERT_FORMAT_NORMAL;
					dwButtonNormalTextID[1] = AP_ID_ALERT_CANCEL_NORMAL;
					dwButtonPressedTextID[0] = AP_ID_ALERT_FORMAT_PRESSED;
					dwButtonPressedTextID[1] = AP_ID_ALERT_CANCEL_PRESSED;

					// ���ļ�ϵͳ����
					XM_OpenAlertView (
						AP_ID_CARD_MESSAGE_CARD_FSERROR,		// ��Ϣ�ı���ԴID
						AP_ID_CARD_ICON_SDCARD,												// ͼƬ��Ϣ��ԴID
						2,												// ��ť����
						dwButtonNormalTextID,					// ��ť������ԴID
						dwButtonPressedTextID,					// ��ť����������ԴID
						APP_ALERT_BKGCOLOR,
						10.0,											// 10�����Զ��ر�
						APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
						fs_access_error_alert_cb,							// �����ص�����
						NULL,											// �û��ص�����˽�в���
						XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
						//XM_VIEW_ALIGN_CENTRE,
						XM_ALERTVIEW_OPTION_ENABLE_CALLBACK	// ALERTVIEW��ͼ�Ŀ���ѡ��
						);
				}
				else if(card_state == DEVCAP_SDCARDSTATE_INVALID)
				{
					// ����Ч(���޷�ʶ��)
					XM_OpenAlertView (
						AP_ID_CARD_MESSAGE_CARD_INVALID,	// ��Ϣ�ı���ԴID
						AP_ID_CARD_ICON_SDCARD,			// ͼƬ��Ϣ��ԴID
						0,
						0,												// ��ť������ԴID
						0,												// ��ť����������ԴID
						APP_ALERT_BKGCOLOR,
						10.0,											// 10�����Զ��ر�
						APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
						NULL,											// �����ص�����
						NULL,											// �û��ص�����˽�в���
						XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
						//XM_VIEW_ALIGN_CENTRE,
						0												// ALERTVIEW��ͼ�Ŀ���ѡ��
						);
				}
				else
				{
					#if 1 //������ֹͣ¼��������²ſ��Խ������ò˵�
					if(!XMSYS_H264CodecGetForcedNonRecordingMode())
					{
				    	// ֹͣ¼��
						XM_OpenAlertView (
							AP_ID_VIDEO_MESSAGE_VIDEO_STOP,			// ��Ϣ�ı���ԴID
							AP_ID_VIDEO_ICON_VIDEOSTOP,				// ͼƬ��Ϣ��ԴID
							0,
							0,										// ��ť������ԴID
							0,										// ��ť����������ԴID
							APP_ALERT_BKGCOLOR,
							(float)1.2,		// �Զ��ر�ʱ��
							APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
							NULL,									// �����ص�����
							NULL,									// �û��ص�����˽�в���
							XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
							//XM_VIEW_ALIGN_CENTRE,
							0										// ALERTVIEW��ͼ�Ŀ���ѡ��
							);
					   return;
					}
					#endif

					// �����Ƶ���б��Ƿ�����ɨ����.
					// ������ɨ��, ����һPOP�Ӵ���ʾ�û�"����ɨ����Ƶ�ļ�,���Ե�"
					if(!XM_VideoItemIsBasicServiceOpened() && !XM_VideoItemIsServiceOpened())
					{
						XM_OpenAlertView (
							AP_ID_VIDEO_VIEW_SCAN_VIDEO_FILE,	// ��Ϣ�ı���ԴID
							AP_ID_CARD_ICON_SDCARD,			// ͼƬ��Ϣ��ԴID
							0,
							0,												// ��ť������ԴID
							0,												// ��ť����������ԴID
							APP_ALERT_BKGCOLOR,
							60.0,											// 60�����Զ��ر�
							APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
							NULL,											// �����ص�����
							NULL,											// �û��ص�����˽�в���
							XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
							//XM_VIEW_ALIGN_CENTRE,
							0												// ALERTVIEW��ͼ�Ŀ���ѡ��
							);

					}
					else
					{
						// ������(��дģʽ��д����ģʽ)
						APPMarkCardChecking (1);		// ��ֹ��Ƶͨ��
						XM_PushWindow (XMHWND_HANDLE(VideoListView));
					}
				}
			}
			else
			{
				#if 1  //������ֹͣ¼��������²ſ��Խ������ò˵�
				if(!XMSYS_H264CodecGetForcedNonRecordingMode())
				{
			    	// ֹͣ¼��
					XM_OpenAlertView (
						AP_ID_VIDEO_MESSAGE_VIDEO_STOP,			// ��Ϣ�ı���ԴID
						AP_ID_VIDEO_ICON_VIDEOSTOP,				// ͼƬ��Ϣ��ԴID
						0,
						0,										// ��ť������ԴID
						0,										// ��ť����������ԴID
						APP_ALERT_BKGCOLOR,
						(float)1.2,		// �Զ��ر�ʱ��
						APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
						NULL,									// �����ص�����
						NULL,									// �û��ص�����˽�в���
						XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
						//XM_VIEW_ALIGN_CENTRE,
						0										// ALERTVIEW��ͼ�Ŀ���ѡ��
						);
				   return;
				}
				#endif

				// �����Ƶ���б��Ƿ�����ɨ����.
				// ������ɨ��, ����һPOP�Ӵ���ʾ�û�"����ɨ����Ƶ�ļ�,���Ե�"
				if(XM_VideoItemIsBasicServiceOpened() && !XM_VideoItemIsServiceOpened())
				{
					XM_OpenAlertView (
							AP_ID_VIDEO_VIEW_SCAN_VIDEO_FILE,	// ��Ϣ�ı���ԴID
							AP_ID_CARD_ICON_SDCARD,			// ͼƬ��Ϣ��ԴID
							0,
							0,												// ��ť������ԴID
							0,												// ��ť����������ԴID
							APP_ALERT_BKGCOLOR,
							60.0,											// 60�����Զ��ر�
							APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
							NULL,											// �����ص�����
							NULL,											// �û��ص�����˽�в���
							XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
							//XM_VIEW_ALIGN_CENTRE,
							0												// ALERTVIEW��ͼ�Ŀ���ѡ��
							);

				}
				else
				{
					APPMarkCardChecking (1);		// ��ֹ��Ƶͨ��
					XM_PushWindow (XMHWND_HANDLE(VideoListView));
				}
			}
			break;
			
		case REMOTE_KEY_UP:
		    if(REMOTE_KEY_RIGHT_ENABLE)
		    {
				REMOTE_KEY_UP_ENABLE = TRUE;
				break;
		    }
		case VK_AP_FONT_BACK_SWITCH:	// �����л���
			//XM_PushWindow (XMHWND_HANDLE(FactoryView));
			{
				unsigned char curch = AP_GetMenuItem(APPMENUITEM_CH);
				XM_printf(">>>>>cur ch:%d\r\n", curch);
				switch(curch)
				{
					case CH_AHD1:
						AP_SetMenuItem(APPMENUITEM_CH, CH_AHD2);
						break;

					case CH_AHD2:
						AP_SetMenuItem(APPMENUITEM_CH, CH_V_AHD12);
						break;

					case CH_V_AHD12:
						AP_SetMenuItem(APPMENUITEM_CH, CH_V_AHD21);
						break;

					case CH_V_AHD21:
						AP_SetMenuItem(APPMENUITEM_CH, CH_H_AHD12);
						break;
						
					case CH_H_AHD12:
						AP_SetMenuItem(APPMENUITEM_CH, CH_H_AHD21);
						break;

					case CH_H_AHD21:
						AP_SetMenuItem(APPMENUITEM_CH, CH_AHD1);
						break;
						
					default:
						break;
				}
				XM_printf(">>>>>setting ch:%d\r\n", AP_GetMenuItem(APPMENUITEM_CH));
			}
			
			if(pipmode == 0)
				pipmode = 1;
			else if(pipmode == 1)
				pipmode = 2;
			else if(pipmode == 2)
				pipmode = 3;
			else if(pipmode == 3)
				pipmode = 4;
			else if(pipmode == 4)
				pipmode = 5;
			else if(pipmode == 5)
				pipmode = 0;
			AppMenuData.AHD_Select = pipmode;
			AP_SaveMenuData (&AppMenuData);
		    #if 0
			   unsigned int mode;
			   if((ITU656_in==1)&&(Reversing==0))  //û�е����źŲ������л�
			   {
					switch (XMSYS_VideoGetImageAssemblyMode())
					{
						case XMSYS_ASSEMBLY_MODE_FRONT_ONLY:
							//mode = XMSYS_ASSEMBLY_MODE_FRONT_REAL;
							mode = XMSYS_ASSEMBLY_MODE_REAL_ONLY;
							break;

						case XMSYS_ASSEMBLY_MODE_FRONT_REAL:
							mode = XMSYS_ASSEMBLY_MODE_REAL_FRONT;
							break;

						case XMSYS_ASSEMBLY_MODE_REAL_FRONT:
							mode = XMSYS_ASSEMBLY_MODE_REAL_ONLY;
							break;

						case XMSYS_ASSEMBLY_MODE_REAL_ONLY:
							mode = XMSYS_ASSEMBLY_MODE_FRONT_ONLY;
							break;

						default:
							mode = XMSYS_ASSEMBLY_MODE_FRONT_REAL;
							break;
					}
					XMSYS_VideoSetImageAssemblyMode (mode);
			   }
			   else if ((ITU656_in==0)&&(Reversing==0))
			   {
                   if(XMSYS_VideoGetImageAssemblyMode()!=XMSYS_ASSEMBLY_MODE_FRONT_ONLY)
                   {
	                   	mode = XMSYS_ASSEMBLY_MODE_FRONT_ONLY;
	                   	XMSYS_VideoSetImageAssemblyMode (mode);
                   }
			   }
        #endif
		break;

		case REMOTE_KEY_DOWN:
		    if(REMOTE_KEY_UP_ENABLE)
			{
				REMOTE_KEY_DOWN_ENABLE = TRUE;
				if(XM_GetFmlDeviceCap(DEVCAP_VIDEO_REC) == DEVCAP_VIDEO_REC_STOP)
                {
    				XM_PushWindow (XMHWND_HANDLE(FactoryView));
    			}
				CleanRemote_KEY_Enable();
			    break;
		    }
		case VK_AP_SWITCH:
			XM_printf(">>>>>>>>>>>>>DesktopOnKeyDown, VK_AP_SWITCH, XMSYS_H264CodecGetForcedNonRecordingMode:%d\r\n", XMSYS_H264CodecGetForcedNonRecordingMode());
			if(XMSYS_H264CodecGetForcedNonRecordingMode())
			{//��¼��ģʽ
			    // �޷��л���¼��ģʽ��������Ϣ���ڣ���ʾԭ��
				unsigned int card_state = XM_GetFmlDeviceCap(DEVCAP_SDCARDSTATE);
				if(card_state == DEVCAP_SDCARDSTATE_UNPLUG)
				{
					// ���Ѱγ�
					XM_OpenAlertView(
						AP_ID_CARD_MESSAGE_CARD_WITHDRAW,			// ��Ϣ�ı���ԴID
						AP_ID_CARD_ICON_SDCARD,						// ͼƬ��Ϣ��ԴID
						0,
						0,											// ��ť������ԴID
						0,											// ��ť����������ԴID
						APP_ALERT_BKGCOLOR,
						2.0,										// 10�����Զ��ر�
						APP_ALERT_BKGALPHA,							// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
						NULL,										// �����ص�����
						NULL,										// �û��ص�����˽�в���
						XM_VIEW_ALIGN_BOTTOM,						// ��ͼ����ʾ����������Ϣ
						0											// ALERTVIEW��ͼ�Ŀ���ѡ��
						);
				}
				else if(card_state == DEVCAP_SDCARDSTATE_FS_ERROR)
				{
					DWORD dwButtonNormalTextID[2];
					DWORD dwButtonPressedTextID[2];
					dwButtonNormalTextID[0] = AP_ID_ALERT_FORMAT_PRESSED;//AP_ID_ALERT_FORMAT_NORMAL;
					dwButtonNormalTextID[1] = AP_ID_ALERT_CANCEL_PRESSED;//AP_ID_ALERT_CANCEL_NORMAL;
					dwButtonPressedTextID[0] = AP_ID_ALERT_FORMAT_PRESSED;
					dwButtonPressedTextID[1] = AP_ID_ALERT_CANCEL_PRESSED;

					// ���ļ�ϵͳ����
					XM_OpenAlertView (
						AP_ID_CARD_MESSAGE_CARD_FSERROR,				// ��Ϣ�ı���ԴID
						AP_ID_CARD_ICON_SDCARD,							// ͼƬ��Ϣ��ԴID
						2,												// ��ť����
						dwButtonNormalTextID,							// ��ť������ԴID
						dwButtonPressedTextID,							// ��ť����������ԴID
						APP_ALERT_BKGCOLOR,
						2.0,											// 10�����Զ��ر�
						APP_ALERT_BKGALPHA,								// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
						fs_access_error_alert_cb,					// �����ص�����
						NULL,											// �û��ص�����˽�в���
						XM_VIEW_ALIGN_BOTTOM,							// ��ͼ����ʾ����������Ϣ
						XM_ALERTVIEW_OPTION_ENABLE_CALLBACK				// ALERTVIEW��ͼ�Ŀ���ѡ��
						);
				}
				else if(card_state == DEVCAP_SDCARDSTATE_INVALID)
				{
					// ����Ч(���޷�ʶ��)
					XM_OpenAlertView (
						AP_ID_CARD_MESSAGE_CARD_INVALID,				// ��Ϣ�ı���ԴID
						AP_ID_CARD_ICON_SDCARD,							// ͼƬ��Ϣ��ԴID
						0,
						0,												// ��ť������ԴID
						0,												// ��ť����������ԴID
						APP_ALERT_BKGCOLOR,
						2.0,											// 10�����Զ��ر�
						APP_ALERT_BKGALPHA,								// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
						NULL,											// �����ص�����
						NULL,											// �û��ص�����˽�в���
						XM_VIEW_ALIGN_BOTTOM,							// ��ͼ����ʾ����������Ϣ
						0												// ALERTVIEW��ͼ�Ŀ���ѡ��
						);
				}
			    else
			    {
			        XMSYS_H264CodecSetForcedNonRecordingMode(0);//���ó�¼��ģʽ,ǿ�Ʒ�¼��ģʽ��¼��ģʽת��,����¼��
					XM_SetFmlDeviceCap(DEVCAP_VIDEO_REC, DEVCAP_VIDEO_REC_START);
			    }
			}
		  	else
			{
			    XMSYS_H264CodecSetForcedNonRecordingMode(1);//���ó�ǿ�Ʒ�¼��ģʽ
				XM_SetFmlDeviceCap(DEVCAP_VIDEO_REC, DEVCAP_VIDEO_REC_STOP);
			}
			break;


        case REMOTE_KEY_RIGHT:
            printf("------------------> REMOTE_KEY_RIGHT\n");
			if(REMOTE_KEY_LEFT_ENABLE)
			{
				REMOTE_KEY_RIGHT_ENABLE = TRUE;
			    break;
			}
		    else
		    {
				CleanRemote_KEY_Enable();
		    }
#if 0
			if(AP_GetMidle_RED_Line()) {
                Red_Location = Red_Location + 1;
                AP_SetRed_Location(Red_Location);
                AP_SaveMenuData (&AppMenuData);
            }
            else {
                if(ACC_Select == 4)
                    break;
                if(APP_GetVOl_OnOff())
		            XM_PushWindow (XMHWND_HANDLE(VolView));
               }
            break;
#endif
		case VK_AP_UP://+��
			XM_printf(">>>>>>>>>>>>>>>>>DesktopOnKeyDown, VK_AP_UP........\r\n");
			{
				DWORD CardStatus = XM_GetFmlDeviceCap (DEVCAP_SDCARDSTATE);
				
				if (CardStatus == DEVCAP_SDCARDSTATE_INSERT)
				{
					AP_OnekeyPhotograph();
				}
			}
			break;
            
        case REMOTE_KEY_LEFT:
            
            printf("------------------> REMOTE_KEY_LEFT\n");
            if(REMOTE_KEY_MENU_ENABLE)
            {
                REMOTE_KEY_LEFT_ENABLE = TRUE;
                break;
            }
            else
            {
                CleanRemote_KEY_Enable();
            }
        #if 0
            if(AP_GetMidle_RED_Line()) {
                Red_Location = Red_Location - 1;
                AP_SetRed_Location(Red_Location);
                AP_SaveMenuData (&AppMenuData);
            }
            else {
                if(ACC_Select == 4)
                    break;
                if(APP_GetVOl_OnOff()) //�����رյ�ʱ��,����Ҫ��������
                    XM_PushWindow (XMHWND_HANDLE(VolView));
            }
            break;
        #endif
		case VK_AP_DOWN:

			break;

#ifdef ISP_RAW_ENABLE
		case VK_F4:
			void isp_write_file_record(char *  addpath  ,unsigned int len );
			//isp_set_work_mode (1);
			OS_Delay (100);
			isp_write_file_record( NULL, 0);
			//isp_set_work_mode (0);
			OS_Delay (100);
			break;
#endif
	}
}

#define POINT_OFFSET   10

VOID DesktopOnKeyLongTimeDown (XMMSG *msg)
{
	XM_printf(">>>>>>>>DesktopOnKeyLongTimeDown, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);
	if(get_trigger_det_status() != 0 || get_delay_return_trig() != TRIGGER_DELAY_END)
    {
        return;
    }
	switch(msg->wp)
	{
	    case REMOTE_KEY_MENU:
	        REMOTE_KEY_MENU_ENABLE = TRUE;
	        break;
		//case VK_AP_UP:		// �л���¼��ط�ģʽ
		case VK_AP_SWITCH:      //�Ļ����,����vcom��������
			//	SetGPIOPadData (GPIO30, euDataLow);//guan anjian shengyin ��������??
			if(iRecordMode == DEMO_MODE)
			{
				// �޷��л�����ƵԤ��ģʽ��������Ϣ���ڣ���ʾԭ��
				unsigned int card_state = XM_GetFmlDeviceCap(DEVCAP_SDCARDSTATE);
				if(card_state == DEVCAP_SDCARDSTATE_UNPLUG)
				{
					// ���Ѱγ�
					XM_OpenAlertView (
						AP_ID_CARD_MESSAGE_CARD_WITHDRAW,	// ��Ϣ�ı���ԴID
						AP_ID_CARD_ICON_SDCARD,			// ͼƬ��Ϣ��ԴID
						0,
						0,												// ��ť������ԴID
						0,												// ��ť����������ԴID
						APP_ALERT_BKGCOLOR,
						10.0,											// 10�����Զ��ر�
						APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
						NULL,											// �����ص�����
						NULL,											// �û��ص�����˽�в���
						XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
						//XM_VIEW_ALIGN_CENTRE,
						0												// ALERTVIEW��ͼ�Ŀ���ѡ��
						);
				}
				else if(card_state == DEVCAP_SDCARDSTATE_FS_ERROR)
				{
					DWORD dwButtonNormalTextID[2];
					DWORD dwButtonPressedTextID[2];
					dwButtonNormalTextID[0] = AP_ID_ALERT_FORMAT_NORMAL;
					dwButtonNormalTextID[1] = AP_ID_ALERT_CANCEL_NORMAL;
					dwButtonPressedTextID[0] = AP_ID_ALERT_FORMAT_PRESSED;
					dwButtonPressedTextID[1] = AP_ID_ALERT_CANCEL_PRESSED;

					// ���ļ�ϵͳ����
					XM_OpenAlertView (
						AP_ID_CARD_MESSAGE_CARD_FSERROR,		// ��Ϣ�ı���ԴID
						AP_ID_CARD_ICON_SDCARD,												// ͼƬ��Ϣ��ԴID
						2,												// ��ť����
						dwButtonNormalTextID,					// ��ť������ԴID
						dwButtonPressedTextID,					// ��ť����������ԴID
						APP_ALERT_BKGCOLOR,
						10.0,											// 10�����Զ��ر�
						APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
						fs_access_error_alert_cb,							// �����ص�����
						NULL,											// �û��ص�����˽�в���
						XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
						//XM_VIEW_ALIGN_CENTRE,
						XM_ALERTVIEW_OPTION_ENABLE_CALLBACK	// ALERTVIEW��ͼ�Ŀ���ѡ��
						);
				}
				else if(card_state == DEVCAP_SDCARDSTATE_INVALID)
				{
					// ����Ч(���޷�ʶ��)
					XM_OpenAlertView (
						AP_ID_CARD_MESSAGE_CARD_INVALID,	// ��Ϣ�ı���ԴID
						AP_ID_CARD_ICON_SDCARD,			// ͼƬ��Ϣ��ԴID
						0,
						0,												// ��ť������ԴID
						0,												// ��ť����������ԴID
						APP_ALERT_BKGCOLOR,
						10.0,											// 10�����Զ��ر�
						APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
						NULL,											// �����ص�����
						NULL,											// �û��ص�����˽�в���
						XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
						//XM_VIEW_ALIGN_CENTRE,
						0												// ALERTVIEW��ͼ�Ŀ���ѡ��
						);
				}
				else
				{
					#if 1 //������ֹͣ¼��������²ſ��Խ������ò˵�
					if(!XMSYS_H264CodecGetForcedNonRecordingMode())
					{
				    	// ֹͣ¼��
						XM_OpenAlertView (
							AP_ID_VIDEO_MESSAGE_VIDEO_STOP,			// ��Ϣ�ı���ԴID
							AP_ID_VIDEO_ICON_VIDEOSTOP,				// ͼƬ��Ϣ��ԴID
							0,
							0,										// ��ť������ԴID
							0,										// ��ť����������ԴID
							APP_ALERT_BKGCOLOR,
							(float)1.2,		// �Զ��ر�ʱ��
							APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
							NULL,									// �����ص�����
							NULL,									// �û��ص�����˽�в���
							XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
							//XM_VIEW_ALIGN_CENTRE,
							0										// ALERTVIEW��ͼ�Ŀ���ѡ��
							);
					   return;
					}
					#endif

					// �����Ƶ���б��Ƿ�����ɨ����.
					// ������ɨ��, ����һPOP�Ӵ���ʾ�û�"����ɨ����Ƶ�ļ�,���Ե�"
					if(!XM_VideoItemIsBasicServiceOpened() && !XM_VideoItemIsServiceOpened())
					{
						XM_OpenAlertView (
							AP_ID_VIDEO_VIEW_SCAN_VIDEO_FILE,	// ��Ϣ�ı���ԴID
							AP_ID_CARD_ICON_SDCARD,			// ͼƬ��Ϣ��ԴID
							0,
							0,												// ��ť������ԴID
							0,												// ��ť����������ԴID
							APP_ALERT_BKGCOLOR,
							60.0,											// 60�����Զ��ر�
							APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
							NULL,											// �����ص�����
							NULL,											// �û��ص�����˽�в���
							XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
							//XM_VIEW_ALIGN_CENTRE,
							0												// ALERTVIEW��ͼ�Ŀ���ѡ��
							);

					}
					else
					{
						// ������(��дģʽ��д����ģʽ)
						APPMarkCardChecking (1);		// ��ֹ��Ƶͨ��
						XM_PushWindow (XMHWND_HANDLE(VideoListView));
					}
				}
			}
			else
			{
				#if 1  //������ֹͣ¼��������²ſ��Խ������ò˵�
				if(!XMSYS_H264CodecGetForcedNonRecordingMode())
				{
			    	// ֹͣ¼��
					XM_OpenAlertView (
						AP_ID_VIDEO_MESSAGE_VIDEO_STOP,			// ��Ϣ�ı���ԴID
						AP_ID_VIDEO_ICON_VIDEOSTOP,				// ͼƬ��Ϣ��ԴID
						0,
						0,										// ��ť������ԴID
						0,										// ��ť����������ԴID
						APP_ALERT_BKGCOLOR,
						(float)1.2,		// �Զ��ر�ʱ��
						APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
						NULL,									// �����ص�����
						NULL,									// �û��ص�����˽�в���
						XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
						//XM_VIEW_ALIGN_CENTRE,
						0										// ALERTVIEW��ͼ�Ŀ���ѡ��
						);
				   return;
				}
				#endif

				// �����Ƶ���б��Ƿ�����ɨ����.
				// ������ɨ��, ����һPOP�Ӵ���ʾ�û�"����ɨ����Ƶ�ļ�,���Ե�"
				if(XM_VideoItemIsBasicServiceOpened() && !XM_VideoItemIsServiceOpened())
				{
					XM_OpenAlertView(
							AP_ID_VIDEO_VIEW_SCAN_VIDEO_FILE,	// ��Ϣ�ı���ԴID
							AP_ID_CARD_ICON_SDCARD,			// ͼƬ��Ϣ��ԴID
							0,
							0,												// ��ť������ԴID
							0,												// ��ť����������ԴID
							APP_ALERT_BKGCOLOR,
							60.0,											// 60�����Զ��ر�
							APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
							NULL,											// �����ص�����
							NULL,											// �û��ص�����˽�в���
							XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
							//XM_VIEW_ALIGN_CENTRE,
							0												// ALERTVIEW��ͼ�Ŀ���ѡ��
							);
				}
				else
				{
					APPMarkCardChecking (1);		// ��ֹ��Ƶͨ��
					if(XM_GetFmlDeviceCap(DEVCAP_SDCARDSTATE) == DEVCAP_SDCARDSTATE_INSERT)
					{
						XM_PushWindow(XMHWND_HANDLE(VideoListView));//��Ƶ6����
						//XM_PushWindow(XMHWND_HANDLE(AlbumListView));//ͼƬ6����
					}
				}
			}
			break;
	}
}

static int start_rec()
{
    int res = -1;
    if(XMSYS_H264CodecGetForcedNonRecordingMode())
	{
	    // �޷��л���¼��ģʽ��������Ϣ���ڣ���ʾԭ��
		unsigned int card_state = XM_GetFmlDeviceCap(DEVCAP_SDCARDSTATE);
		if(card_state == DEVCAP_SDCARDSTATE_UNPLUG)
		{
			// ���Ѱγ�
			XM_OpenAlertView (
				AP_ID_CARD_MESSAGE_CARD_WITHDRAW,			// ��Ϣ�ı���ԴID
				AP_ID_CARD_ICON_SDCARD,						// ͼƬ��Ϣ��ԴID
				0,
				0,											// ��ť������ԴID
				0,											// ��ť����������ԴID
				APP_ALERT_BKGCOLOR,
				10.0,										// 10�����Զ��ر�
				APP_ALERT_BKGALPHA,							// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
				NULL,										// �����ص�����
				NULL,										// �û��ص�����˽�в���
				XM_VIEW_ALIGN_BOTTOM,						// ��ͼ����ʾ����������Ϣ
				0											// ALERTVIEW��ͼ�Ŀ���ѡ��
				);
		}
		else if(card_state == DEVCAP_SDCARDSTATE_FS_ERROR)
		{
			DWORD dwButtonNormalTextID[2];
			DWORD dwButtonPressedTextID[2];
			dwButtonNormalTextID[0] = AP_ID_ALERT_FORMAT_PRESSED;//AP_ID_ALERT_FORMAT_NORMAL;
			dwButtonNormalTextID[1] = AP_ID_ALERT_CANCEL_PRESSED;//AP_ID_ALERT_CANCEL_NORMAL;
			dwButtonPressedTextID[0] = AP_ID_ALERT_FORMAT_PRESSED;
			dwButtonPressedTextID[1] = AP_ID_ALERT_CANCEL_PRESSED;

			// ���ļ�ϵͳ����
			XM_OpenAlertView (
				AP_ID_CARD_MESSAGE_CARD_FSERROR,				// ��Ϣ�ı���ԴID
				AP_ID_CARD_ICON_SDCARD,							// ͼƬ��Ϣ��ԴID
				2,												// ��ť����
				dwButtonNormalTextID,							// ��ť������ԴID
				dwButtonPressedTextID,							// ��ť����������ԴID
				APP_ALERT_BKGCOLOR,
				10.0,											// 10�����Զ��ر�
				APP_ALERT_BKGALPHA,								// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
				fs_access_error_alert_cb,					// �����ص�����
				NULL,											// �û��ص�����˽�в���
				XM_VIEW_ALIGN_BOTTOM,							// ��ͼ����ʾ����������Ϣ
				XM_ALERTVIEW_OPTION_ENABLE_CALLBACK				// ALERTVIEW��ͼ�Ŀ���ѡ��
				);
		}
		else if(card_state == DEVCAP_SDCARDSTATE_INVALID)
		{
			// ����Ч(���޷�ʶ��)
			XM_OpenAlertView (
				AP_ID_CARD_MESSAGE_CARD_INVALID,				// ��Ϣ�ı���ԴID
				AP_ID_CARD_ICON_SDCARD,							// ͼƬ��Ϣ��ԴID
				0,
				0,												// ��ť������ԴID
				0,												// ��ť����������ԴID
				APP_ALERT_BKGCOLOR,
				10.0,											// 10�����Զ��ر�
				APP_ALERT_BKGALPHA,								// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
				NULL,											// �����ص�����
				NULL,											// �û��ص�����˽�в���
				XM_VIEW_ALIGN_BOTTOM,							// ��ͼ����ʾ����������Ϣ
				0												// ALERTVIEW��ͼ�Ŀ���ѡ��
				);
		}
	    else
	    {
	        XMSYS_H264CodecSetForcedNonRecordingMode(0);
			XM_SetFmlDeviceCap (DEVCAP_VIDEO_REC, DEVCAP_VIDEO_REC_START);
			res = 0;
	    }
	}
	return res;
}

static int stop_rec()
{
    int res = -1;
    if(XMSYS_H264CodecGetForcedNonRecordingMode() == 0)
    {
        XMSYS_H264CodecSetForcedNonRecordingMode(1);
    	XM_SetFmlDeviceCap(DEVCAP_VIDEO_REC, DEVCAP_VIDEO_REC_STOP);
    	res = 0;
    }
    return res;
}
/*
OnSystemEvent������ϵͳ�¼��Ĵ���
	�ļ�ϵͳ�쳣�����ʽ�����쳣��
	�ɼ�¼ʱ���
	���ݵ�ص�ѹ�͵����������̴����г������в�����ʾ��

	�����룬������OnEnterһ���ļ�飬��ʾ��¼ʱ�䣬¼��״̬����Ϊ����ͷ����״̬�û��Ѿ�֪��������ʾ�Ƿ񲿷�����ͷ�Ƿ�����

	���γ�����ʾ״̬�£������κδ����г���¼״̬�£��ر��г���¼��������ʾ���棬�ȴ��û�ȷ�ϡ��˳��طŽ���ʱ������OnEnter�ٴ��жϸý�����һ��״̬

	������ͷ�����жϣ�������ʾ����¼״̬��

	�ļ�ϵͳ�쳣���г���¼״̬�£��ر��г���¼�����½���OnEnter��������Ӧ����

	ʱ�����㣺������ʾ
*/

VOID XM_DefaultSystemEventProc (HANDLE hWnd, XMMSG *msg)
{
	//HANDLE hDesktop = XM_GetDesktop ();
	XM_BreakSystemEventDefaultProcess (msg);
	switch(msg->wp)
	{
		// ------------------------------------
		//
		// *** ���¼����� ***
		//
		// ------------------------------------
		case SYSTEM_EVENT_CARD_DETECT:			// SD��׼����(SD�������)
			// �������(��⵽���Ѳ��룬����ʶ����)����ʾ�û�
#if CZS_USB_01

#else
			XM_OpenAlertView (
							AP_ID_CARD_MESSAGE_CARD_DETECT,	// ��Ϣ�ı���ԴID
							AP_ID_CARD_ICON_SDCARD,			// ͼƬ��Ϣ��ԴID
							0,
							0,												// ��ť������ԴID
							0,												// ��ť����������ԴID
							APP_ALERT_BKGCOLOR,
							2.0,											// 5�����Զ��ر�
							APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
							NULL,											// �����ص�����
							NULL,											// �û��ص�����˽�в���
							XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
							//XM_VIEW_ALIGN_CENTRE,
							0												// ALERTVIEW��ͼ�Ŀ���ѡ��
							);
#endif
			break;

		case SYSTEM_EVENT_CARD_UNPLUG:			// SD���γ��¼�
			XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_CARD_UNPLUG................\r\n");
			// ��ǿ����¼��
			XMSYS_UvcSocketReportSDCardSystemEvent(SYSTEM_EVENT_CARD_UNPLUG);
			APPMarkCardChecking(0);
			XM_AppInitStartupTicket();  //������γ�����ʱʱ�����¼�
			XMSYS_H264CodecSetForcedNonRecordingMode(1);//���ó�ǿ�Ʒ�¼��ģʽ
			#if 0
			/*
			if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
				XM_voice_prompts_insert_voice (XM_VOICE_ID_ALARM);
			*/
#if CZS_USB_01
#else
			XM_OpenAlertView (
					AP_ID_CARD_MESSAGE_CARD_WITHDRAW,	// ��Ϣ�ı���ԴID
					AP_ID_CARD_ICON_SDCARD,			// ͼƬ��Ϣ��ԴID
					0,
					0,												// ��ť������ԴID
					0,												// ��ť����������ԴID
					APP_ALERT_BKGCOLOR,
					2.0,											// 5�����Զ��ر�
					APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
					NULL,											// �����ص�����
					NULL,											// �û��ص�����˽�в���
					XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
					//XM_VIEW_ALIGN_CENTRE,
					0												// ALERTVIEW��ͼ�Ŀ���ѡ��
					);
#endif
			#endif
			break;

		case SYSTEM_EVENT_CARD_FS_ERROR:					// �ļ�ϵͳ�쳣�¼�(�޷�ʶ������ݿ�)
			XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_CARD_FS_ERROR................\r\n");
			#if 0
			XMSYS_UvcSocketReportSDCardSystemEvent (SYSTEM_EVENT_CARD_FS_ERROR);
			APPMarkCardChecking	(0);
			/*
			// ��ǿ����¼��
			if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
				XM_voice_prompts_insert_voice (XM_VOICE_ID_ALARM);
			*/
#if CZS_USB_01

#else
			{
					DWORD dwButtonNormalTextID[2];
					DWORD dwButtonPressedTextID[2];
					dwButtonNormalTextID[0] = AP_ID_ALERT_FORMAT_NORMAL;
					dwButtonNormalTextID[1] = AP_ID_ALERT_CANCEL_NORMAL;
					dwButtonPressedTextID[0] = AP_ID_ALERT_FORMAT_PRESSED;
					dwButtonPressedTextID[1] = AP_ID_ALERT_CANCEL_PRESSED;
					XM_printf ("FS_ERROR\n");
					// ���ļ�ϵͳ����
					XM_OpenAlertView (
						AP_ID_CARD_MESSAGE_CARD_FSERROR,		// ��Ϣ�ı���ԴID
						AP_NULLID, // AP_ID_CARD_ICON_SDCARD,												// ͼƬ��Ϣ��ԴID
						2,												// ��ť����
						dwButtonNormalTextID,					// ��ť������ԴID
						dwButtonPressedTextID,					// ��ť����������ԴID
						APP_ALERT_BKGCOLOR,
						10.0,											// 30�����Զ��ر�
						APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
						fs_access_error_alert_cb,							// �����ص�����
						NULL,											// �û��ص�����˽�в���
						XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
						//XM_VIEW_ALIGN_CENTRE,
						XM_ALERTVIEW_OPTION_ENABLE_CALLBACK	// ALERTVIEW��ͼ�Ŀ���ѡ��
						);
			}
#endif
			#endif
			break;

		case SYSTEM_EVENT_CARD_VERIFY_ERROR:	// ����дУ��ʧ��(���������𻵻��߿��ļ�ϵͳ�쳣)
			XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_CARD_VERIFY_ERROR................\r\n");
			// ��鿨�Ƿ����, ��ֹ�ٱ���
			if(!XMSYS_check_sd_card_exist())
				break;
			XMSYS_UvcSocketReportSDCardSystemEvent (SYSTEM_EVENT_CARD_VERIFY_ERROR);

			if(XM_GetTickCount() < ticket_to_card_verify_warning)
				break;

			ticket_to_card_verify_warning = XM_GetTickCount() + 10*1000;

			// ����¼��
			if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
				XM_voice_prompts_insert_voice (XM_VOICE_ID_ALARM );
#if CZS_USB_01
#else
			// �л�������Ϣ�Ӵ�
			XM_OpenAlertView (
					AP_ID_CARD_MESSAGE_CARD_VERIFYFAILED,	// ��Ϣ�ı���ԴID
					AP_ID_CARD_ICON_SDCARD,												// ͼƬ��Ϣ��ԴID
					0,												// ��ť����
					0,												// ��ť������ԴID
					0,												// ��ť����������ԴID
					APP_ALERT_BKGCOLOR,
					10.0,											// 10�����Զ��ر�
					APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
					NULL,											// �����ص�����
					NULL,											// �û��ص�����˽�в���
					XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
					//XM_VIEW_ALIGN_CENTRE,
					0												// ALERTVIEW��ͼ�Ŀ���ѡ��
					);
#endif
			break;


		case SYSTEM_EVENT_CARD_INSERT:			// SD�������¼�(��дģʽ���ļ�ϵͳ���OK)
			XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_CARD_INSERT................\r\n");
			XM_printf(">XMSYS_H264CodecGetForcedNonRecordingMode():%d\r\n", XMSYS_H264CodecGetForcedNonRecordingMode());

			#if 1
			// ���¼��״̬
			//XMSYS_UvcSocketReportSDCardSystemEvent(SYSTEM_EVENT_CARD_INSERT);
			if(XMSYS_H264CodecGetForcedNonRecordingMode()==0)
			{
				XMSYS_H264CodecSetForcedNonRecordingMode(1);//���ó�ǿ�Ʒ�¼��ģʽ
			}
			#endif
			
			#if 0
			if(XMSYS_H264CodecGetForcedNonRecordingMode())
			{
			    XMSYS_H264CodecSetForcedNonRecordingMode(0);
			}

			XM_SetFmlDeviceCap (DEVCAP_VIDEO_REC, DEVCAP_VIDEO_REC_START);
			#endif
			
			//XM_SetFmlDeviceCap(DEVCAP_VIDEO_REC, DEVCAP_VIDEO_REC_START);
			/*
			// ��ǿ����¼��
			if(AP_GetMenuItem (APPMENUITEM_VOICEASSISTANT_CARD_STATUS))
				XM_voice_prompts_insert_voice (XM_VOICE_ID_CARD_INSERT);
			*/

			#if 0
			XM_OpenAlertView (
					AP_ID_CARD_MESSAGE_CARD_INSERT,	// ��Ϣ�ı���ԴID
					AP_ID_CARD_ICON_SDCARD,			// ͼƬ��Ϣ��ԴID
					0,
					0,												// ��ť������ԴID
					0,												// ��ť����������ԴID
					APP_ALERT_BKGCOLOR,
					5.0,											// 5�����Զ��ر�
					APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
					NULL,											// �����ص�����
					NULL,											// �û��ص�����˽�в���
					XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
					//XM_VIEW_ALIGN_CENTRE,
					0												// ALERTVIEW��ͼ�Ŀ���ѡ��
					);
			#endif
			//DesktopOnSystemCheck();
			ticket_to_card_verify_warning = 0;
			break;

		case SYSTEM_EVENT_CARD_INVALID:			// SD���޷�ʶ��
			XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_CARD_INVALID................\r\n");
			//	SD���޷�ʶ�� (��⵽�����룬����û��Ӧ�𿨿ص��κ�ָ��)
			XMSYS_UvcSocketReportSDCardSystemEvent (SYSTEM_EVENT_CARD_INVALID);
			//if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
			//	XM_voice_prompts_insert_voice (XM_VOICE_ID_ALARM);
#if CZS_USB_01
#else
			XM_OpenAlertView (
					AP_ID_CARD_MESSAGE_CARD_INVALID,	// ��Ϣ�ı���ԴID
					AP_ID_CARD_ICON_SDCARD,			// ͼƬ��Ϣ��ԴID
					0,
					0,												// ��ť������ԴID
					0,												// ��ť����������ԴID
					APP_ALERT_BKGCOLOR,
					10.0,											// 10�����Զ��ر�
					APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
					NULL,											// �����ص�����
					NULL,											// �û��ص�����˽�в���
					XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
					//XM_VIEW_ALIGN_CENTRE,
					0												// ALERTVIEW��ͼ�Ŀ���ѡ��
					);
#endif
			break;

		case SYSTEM_EVENT_CARD_DISK_FULL:		// SD����������
			XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_CARD_DISK_FULL................\r\n");
			XMSYS_UvcSocketReportSDCardSystemEvent (SYSTEM_EVENT_CARD_DISK_FULL);
			if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
				XM_voice_prompts_insert_voice (XM_VOICE_ID_ALARM);
#if CZS_USB_01
#else
			XM_OpenAlertView (
					AP_ID_CARD_MESSAGE_CARD_DISK_FULL,	// ��Ϣ�ı���ԴID
					AP_ID_CARD_ICON_SDCARD,			// ͼƬ��Ϣ��ԴID
					0,
					0,												// ��ť������ԴID
					0,												// ��ť����������ԴID
					APP_ALERT_BKGCOLOR,
					20.0,											// 10�����Զ��ر�
					APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
					NULL,											// �����ص�����
					NULL,											// �û��ص�����˽�в���
					XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
					//XM_VIEW_ALIGN_CENTRE,
					0												// ALERTVIEW��ͼ�Ŀ���ѡ��
					);
#endif
			break;

		// ------------------------------------
		//
		// *** �ļ�ϵͳ����Ƶ��ϵͳ�¼����� ***
		//
		// ------------------------------------
		case SYSTEM_EVENT_VIDEOITEM_LOW_PREFORMANCE:		// �ļ�ϵͳ�Ĳ�����Ҫ�������ã�ȷ��������Ƶ��¼��Ҫ��
															// һ��ָSD���Ĵش�С̫С�����¿���д�ٶ��޷����������Ƶʵʱ��д��
		XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_VIDEOITEM_LOW_PREFORMANCE................\r\n");
		{
			DWORD dwButtonNormalTextID[2];
			DWORD dwButtonPressedTextID[2];
			dwButtonNormalTextID[0] = AP_ID_ALERT_FORMAT_NORMAL;
			dwButtonNormalTextID[1] = AP_ID_ALERT_CANCEL_NORMAL;
			dwButtonPressedTextID[0] = AP_ID_ALERT_FORMAT_PRESSED;
			dwButtonPressedTextID[1] = AP_ID_ALERT_CANCEL_PRESSED;

			XMSYS_UvcSocketReportSDCardSystemEvent (SYSTEM_EVENT_VIDEOITEM_LOW_PREFORMANCE);
#if CZS_USB_01
#else
			XM_OpenAlertView (
					AP_ID_CARD_INFO_LOWPREFORMANCE,	// ��Ϣ�ı���ԴID
					AP_NULLID,	//AP_ID_CARD_ICON_SDCARD,			// ͼƬ��Ϣ��ԴID
					2,												// ��ť����
					dwButtonNormalTextID,					// ��ť������ԴID
					dwButtonPressedTextID,					// ��ť����������ԴID
					APP_ALERT_BKGCOLOR,
					15.0,											// 15�����Զ��ر�
					APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
					fs_low_performance_alert_cb,			// �����ص�����
					NULL,											// �û��ص�����˽�в���
					XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
					//XM_VIEW_ALIGN_CENTRE,
					XM_ALERTVIEW_OPTION_ENABLE_CALLBACK			// ALERTVIEW��ͼ�Ŀ���ѡ��
					);
#endif
			break;
		}

		case SYSTEM_EVENT_VIDEOITEM_RECYCLE_CONSUMED:		// "������¼���ļ��������޷�ѭ��¼����ɾ���ļ������¸�ʽ��"
			XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_VIDEOITEM_RECYCLE_CONSUMED................\r\n");
			XMSYS_UvcSocketReportSDCardSystemEvent (SYSTEM_EVENT_VIDEOITEM_RECYCLE_CONSUMED);
#if CZS_USB_01
#else
			XM_OpenAlertView (
					AP_ID_CARD_INFO_RECYCLE_CONSUMED,	// ��Ϣ�ı���ԴID
					AP_ID_CARD_ICON_SDCARD,			// ͼƬ��Ϣ��ԴID
					0,
					0,												// ��ť������ԴID
					0,												// ��ť����������ԴID
					APP_ALERT_BKGCOLOR,
					5.0,											// 5�����Զ��ر�
					APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
					NULL,											// �����ص�����
					NULL,											// �û��ص�����˽�в���
					XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
					//XM_VIEW_ALIGN_CENTRE,
					0												// ALERTVIEW��ͼ�Ŀ���ѡ��
					);
#endif
			break;

		case SYSTEM_EVENT_VIDEOITEM_LOW_SPEED:			// "SD��д���ٶȽ�����������Ҫ���¸�ʽ��"
			XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_VIDEOITEM_LOW_SPEED................\r\n");

			XMSYS_UvcSocketReportSDCardSystemEvent (SYSTEM_EVENT_VIDEOITEM_LOW_PREFORMANCE);
#if CZS_USB_01
#else
			XM_OpenAlertView (
					AP_ID_CARD_INFO_LOWSPEED,	// ��Ϣ�ı���ԴID
					AP_ID_CARD_ICON_SDCARD,			// ͼƬ��Ϣ��ԴID
					0,
					0,												// ��ť������ԴID
					0,												// ��ť����������ԴID
					APP_ALERT_BKGCOLOR,
					5.0,											// 5�����Զ��ر�
					APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
					NULL,											// �����ص�����
					NULL,											// �û��ص�����˽�в���
					XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
					//XM_VIEW_ALIGN_CENTRE,
					0												// ALERTVIEW��ͼ�Ŀ���ѡ��
					);
#endif
			break;

		case SYSTEM_EVENT_VIDEOITEM_LOW_SPACE:			// ѭ��¼��Ŀռ䲻��, ��ɾ�������ļ����ʽ��
			XMSYS_UvcSocketReportSDCardSystemEvent (SYSTEM_EVENT_CARD_DISK_FULL);
			
			XM_OpenAlertView (
					AP_ID_CARD_INFO_LOWSPACE,	// ��Ϣ�ı���ԴID
					AP_ID_CARD_ICON_SDCARD,			// ͼƬ��Ϣ��ԴID
					0,
					0,												// ��ť������ԴID
					0,												// ��ť����������ԴID
					APP_ALERT_BKGCOLOR,
					5.0,											// 5�����Զ��ر�
					APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
					NULL,											// �����ص�����
					NULL,											// �û��ص�����˽�в���
					XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
					//XM_VIEW_ALIGN_CENTRE,
					0												// ALERTVIEW��ͼ�Ŀ���ѡ��
					);
			
#if CZS_USB_01
#else
			XM_OpenRecycleVideoAlertView (
				AP_ID_CARD_INFO_LOWSPACE,		// ѭ��¼��ʱ������趨ֵ
				APP_ALERT_BKGCOLOR,						// ����ɫ
				10.0,											// 10�����Զ��ر�
				APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
				XM_VIEW_ALIGN_BOTTOM,
				0
				);
#endif
			break;
#if 0
		case SYSTEM_EVENT_CCD0_LOST_CONNECT:		// ����ͷ���ϣ��޷�ʶ��
			XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_CCD0_LOST_CONNECT................\r\n");
			XM_voice_prompts_insert_voice (XM_VOICE_ID_ALARM );
#if CZS_USB_01
#else
			XM_OpenAlertView (
					AP_ID_CAMERA_INFO_CAMERADISCONNECT,	// ��Ϣ�ı���ԴID
					AP_ID_CAMERA_ICON_CAMERADISCONNECT,	// ͼƬ��Ϣ��ԴID
					0,
					0,												// ��ť������ԴID
					0,												// ��ť����������ԴID
					APP_ALERT_BKGCOLOR,
					60.0,											// 60�����Զ��ر�
					APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
					NULL,											// �����ص�����
					NULL,											// �û��ص�����˽�в���
					XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
					0												// ALERTVIEW��ͼ�Ŀ���ѡ��
					);
#endif
			break;
#endif
		case SYSTEM_EVENT_ADJUST_BELL_VOLUME:
			XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_ADJUST_BELL_VOLUME................\r\n");
			XM_OpenBellSoundVolumeSettingView ();
			break;

		case SYSTEM_EVENT_ADJUST_MIC_VOLUME:
			XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_ADJUST_MIC_VOLUME................\r\n");
			if(XM_GetFmlDeviceCap(DEVCAP_SCREEN) == DEVCAP_SCREEN_DISCONNECT)
			{
				// ��ʾ��������
				// TOGGLE�л�ģʽ
				if(AP_GetMenuItem (APPMENUITEM_MIC) == 0)
				{
					// MIC�ѹر�
					// ����MIC¼��
					AP_SetMenuItem (APPMENUITEM_MIC, 1);
				}
				else
				{
					// MIC�ѿ���
					// �ر�MIC
					AP_SetMenuItem (APPMENUITEM_MIC, 0);
				}
			}
			else
			{
				// ��ʾ��δ����
				XM_OpenMicSoundVolumeSettingView ();
			}
			break;

		// ����ص�ѹ�仯�¼�
		case SYSTEM_EVENT_MAIN_BATTERY:
		XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_MAIN_BATTERY................\r\n");
		{
			// ��ȡ����صĵ�ǰ״̬
			//XM_GetFmlDeviceCap (DEVCAP_MAINBATTERYVOLTAGE);
			int main_battery_voltage_level = battery_get_lvl();
			XM_printf ("main battery level=%d\n", main_battery_voltage_level);
			if(main_battery_voltage_level <= DEVCAP_VOLTAGE_WORST)
			{
			    //start_rec();
			    stop_rec();
#if CZS_USB_01
#else
				XM_OpenAlertView (
					AP_ID_MAINBATTERY_INFO_WORST,			// ��Ϣ�ı���ԴID
					AP_ID_BATTERY_ICON,						// ͼƬ��Ϣ��ԴID
					0,
					0,												// ��ť������ԴID
					0,												// ��ť����������ԴID
					APP_ALERT_BKGCOLOR,
					10.0,											// 10�����Զ��ر�
					APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
					NULL,											// �����ص�����
					NULL,											// �û��ص�����˽�в���
					XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
					//XM_VIEW_ALIGN_CENTRE,
					0												// ALERTVIEW��ͼ�Ŀ���ѡ��
					);
#endif
			}
			break;
		}
		case SYSTEM_EVENT_ACC_CONNECT:
		    if(XM_GetFmlDeviceCap(DEVCAP_VIDEO_REC) == DEVCAP_VIDEO_REC_START)
	        {
	            int res = stop_rec();
	        }
		    start_rec();
		    dettask_send_mail(0);
		    break;
		case SYSTEM_EVENT_ACC_LOST_CONNECT:
		    XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_ACC_LOST_CONNECT................\r\n");
		    //start_rec();
            //���acc�Ͽ������ƶ����ѡ��Ϊ���������ʼ������ƶ���⹦��
    		dettask_send_mail(1);
		    break;
			
        case SYSTEM_EVENT_SHUTDOWN_SOON:
			if(XM_GetFmlDeviceCap(DEVCAP_VIDEO_REC) == DEVCAP_VIDEO_REC_START)
	        {
	            printf("---------------------> SYSTEM_EVENT_SHUTDOWN_SOON\n");
	            int res = stop_rec();
	            printf("---------------------> StopRecord res : %d\n",res);
	        }
			break;
			
		case SYSTEM_EVENT_USB_DISCONNECT:		// USB���ӶϿ�
			XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_USB_DISCONNECT................\r\n");
#if CZS_USB_01
#else
			XM_OpenAlertView (
					AP_ID_USB_DISCONNECT_TITLE,			// ��Ϣ�ı���ԴID
					AP_ID_USB_DISCONNECT_IMAGE,			// ͼƬ��Ϣ��ԴID
					0,
					0,												// ��ť������ԴID
					0,												// ��ť����������ԴID
					APP_ALERT_BKGCOLOR,
					5.0,											// 5�����Զ��ر�
					APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
					NULL,											// �����ص�����
					NULL,											// �û��ص�����˽�в���
					XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
					//XM_VIEW_ALIGN_CENTRE,
					0												// ALERTVIEW��ͼ�Ŀ���ѡ��
					);
#endif
			break;

		case SYSTEM_EVENT_ONE_KEY_PROTECT:			// ��ݰ����¼�������¼��(һ������)
			XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_ONE_KEY_PROTECT................\r\n");
			{
				int urgent_record_time;
				DWORD dwButtonNormalTextID[1];
				DWORD dwButtonPressedTextID[1];
				// ��ȡ����¼��ʱ������
				if(XMSYS_H264CodecGetMode() != XMSYS_H264_CODEC_MODE_RECORD)
					break;
				urgent_record_time = AP_GetMenuItem (APPMENUITEM_URGENT_RECORD_TIME);
				urgent_record_time = (urgent_record_time + 1) * 5 * 60;
				dwButtonNormalTextID[0] = AP_ID_ALERT_BUTTON_OK_NORMAL;
				dwButtonPressedTextID[0] = AP_ID_ALERT_BUTTON_OK_PRESSED;

#if CZS_USB_01
#else
				XM_OpenAlertView (AP_ID_URGENT_RECORD_MESSAGE,
											AP_ID_URGENT_RECORD_IMAGE,
											1,
											dwButtonNormalTextID,
											dwButtonPressedTextID,
											APP_ALERT_BKGCOLOR,
											//0xE0C0C0C0,
											(float)urgent_record_time,		// �Զ��ر�ʱ��
											APP_ALERT_BKGALPHA,
											//XM_VIEW_ALIGN_BOTTOM
											NULL,											// �����ص�����
											NULL,											// �û��ص�����˽�в���
											XM_VIEW_ALIGN_CENTRE,
											XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN|XM_ALERTVIEW_OPTION_ADJUST_COUNTDOWN
											);
#endif
				break;
			}

		case SYSTEM_EVENT_ONE_KEY_PHOTOGRAPH:// ��ݰ����¼���һ������
			XM_printf(">>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_ONE_KEY_PHOTOGRAPH............\r\n");
			{
				// ������ʾ����
				unsigned int card_state = XM_GetFmlDeviceCap(DEVCAP_SDCARDSTATE);
					if(card_state == DEVCAP_SDCARDSTATE_UNPLUG)
					{
						// ���Ѱγ�
						XM_OpenAlertView (
							AP_ID_CARD_MESSAGE_CARD_WITHDRAW,	// ��Ϣ�ı���ԴID
							AP_ID_CARD_ICON_SDCARD,			// ͼƬ��Ϣ��ԴID
							0,
							0,												// ��ť������ԴID
							0,												// ��ť����������ԴID
							APP_ALERT_BKGCOLOR,
							10.0,											// 10�����Զ��ر�
							APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
							NULL,											// �����ص�����
							NULL,											// �û��ص�����˽�в���
							XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
							//XM_VIEW_ALIGN_CENTRE,
							0												// ALERTVIEW��ͼ�Ŀ���ѡ��
							);
					}
					else if(card_state == DEVCAP_SDCARDSTATE_FS_ERROR)
					{
						DWORD dwButtonNormalTextID[2];
						DWORD dwButtonPressedTextID[2];
						dwButtonNormalTextID[0] = AP_ID_ALERT_FORMAT_NORMAL;
						dwButtonNormalTextID[1] = AP_ID_ALERT_CANCEL_NORMAL;
						dwButtonPressedTextID[0] = AP_ID_ALERT_FORMAT_PRESSED;
						dwButtonPressedTextID[1] = AP_ID_ALERT_CANCEL_PRESSED;

						// ���ļ�ϵͳ����
						XM_OpenAlertView (
							AP_ID_CARD_MESSAGE_CARD_FSERROR,		// ��Ϣ�ı���ԴID
							AP_ID_CARD_ICON_SDCARD,												// ͼƬ��Ϣ��ԴID
							2,												// ��ť����
							dwButtonNormalTextID,					// ��ť������ԴID
							dwButtonPressedTextID,					// ��ť����������ԴID
							APP_ALERT_BKGCOLOR,
							10.0,											// 10�����Զ��ر�
							APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
							fs_access_error_alert_cb,							// �����ص�����
							NULL,											// �û��ص�����˽�в���
							XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
							//XM_VIEW_ALIGN_CENTRE,
							XM_ALERTVIEW_OPTION_ENABLE_CALLBACK	// ALERTVIEW��ͼ�Ŀ���ѡ��
							);
					}
					else if(card_state == DEVCAP_SDCARDSTATE_INVALID)
					{
						// ����Ч(���޷�ʶ��)
						XM_OpenAlertView (
							AP_ID_CARD_MESSAGE_CARD_INVALID,	// ��Ϣ�ı���ԴID
							AP_ID_CARD_ICON_SDCARD,			// ͼƬ��Ϣ��ԴID
							0,
							0,												// ��ť������ԴID
							0,												// ��ť����������ԴID
							APP_ALERT_BKGCOLOR,
							10.0,											// 10�����Զ��ر�
							APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
							NULL,											// �����ص�����
							NULL,											// �û��ص�����˽�в���
							XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
							//XM_VIEW_ALIGN_CENTRE,
							0												// ALERTVIEW��ͼ�Ŀ���ѡ��
							);
					}
					else
					{
						int ret;
						//PhtographStart=1;
						ret = XMSYS_JpegCodecOneKeyPhotograph();
						if(ret == 0)
						{
							XM_voice_prompts_insert_voice(XM_VOICE_ID_ONEKEY_PHOTOGRAPH);
						}

		                //XM_PeekMessage (msg, XM_TOUCHDOWN, XM_TOUCHUP);
						//XM_PeekMessage (msg, XM_KEYDOWN, XM_KEYUP);
						//�������ڼ����������������ΰ�������������
						//PhtographStart=0;
						XM_FlushMessage ();
					}
				break;
			}

		#if 0
		case SYSTEM_EVENT_GPSBD_DISCONNECT:					// GPSBD�ѶϿ�
			XM_OpenAlertView (
					AP_ID_GPSBD_DISCONNECT,					// ��Ϣ�ı���ԴID
					AP_ID_GPSBD_ICON,							// ͼƬ��Ϣ��ԴID
					0,												// ��ť����
					0,												// ��ť������ԴID
					0,												// ��ť����������ԴID
					APP_ALERT_BKGCOLOR,
					5.0,											// 5�����Զ��ر�
					APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
					NULL,											// �����ص�����
					NULL,											// �û��ص�����˽�в���
					XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
					//XM_VIEW_ALIGN_CENTRE,
					0												// ALERTVIEW��ͼ�Ŀ���ѡ��
					);
			break;
		case SYSTEM_EVENT_GPSBD_CONNECT_LOCATE_OK:		// GPSBD�Ѷ�λ
			XM_OpenAlertView (
					AP_ID_GPSBD_CONNECT_LOCATE_OK,	// ��Ϣ�ı���ԴID
					AP_ID_GPSBD_ICON,						// ͼƬ��Ϣ��ԴID
					0,												// ��ť����
					0,												// ��ť������ԴID
					0,												// ��ť����������ԴID
					APP_ALERT_BKGCOLOR,
					5.0,											// 5�����Զ��ر�
					APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
					NULL,											// �����ص�����
					NULL,											// �û��ص�����˽�в���
					XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
					//XM_VIEW_ALIGN_CENTRE,
					0												// ALERTVIEW��ͼ�Ŀ���ѡ��
					);
			break;
		#endif
		
		case SYSTEM_EVENT_SYSTEM_UPDATE_FILE_CHECKED:	// �ҵ��Ϸ���ϵͳ�����ļ�
			XM_printf(">>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_SYSTEM_UPDATE_FILE_CHECKED............\r\n");
			//OpenSystemUpdateDialog ();
			AP_OpenSystemUpdateView(0);
			break;

		case SYSTEM_EVENT_BL_OFF:			// �ر���
			XM_printf(">>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_BL_OFF............\r\n");

			#if 1
			{
				// OSD�ر�
				XM_printf ("SYSTEM_EVENT_BL_OFF\n");
				//XM_SetFmlDeviceCap (DEVCAP_OSD, 0);
				// �رձ���
				XM_SetFmlDeviceCap (DEVCAP_BACKLIGHT, 0);

				XM_printf ("BL off\n");
			}
			#endif
			break;

			
		case SYSTEM_EVENT_BL_ON:			// ������
			XM_printf(">>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_BL_ON............\r\n");
			{
				XM_printf ("BL on\n");
				// OSD����
				//XM_SetFmlDeviceCap (DEVCAP_OSD, 1);
				// ���⿪��
				XM_SetFmlDeviceCap (DEVCAP_BACKLIGHT, 1);
			}
			break;

		default:
			break;
	}

}


static void SystemUpdateDialogCallBack (VOID *UserPrivate, UINT uKeyPressed)
{
	if(uKeyPressed == VK_AP_MENU)	// YES
	{
		// ִ��"ϵͳ����"����
		AP_OpenSystemUpdateView (0);
		return;
	}
	// ��alert����
	XM_PullWindow (0);
}

// ��ϵͳ������ʾ�Ի���
void OpenSystemUpdateDialog (void)
{
	// 2������, YES/CANCEL
	DWORD dwButtonNormalTextID[2];
	DWORD dwButtonPressedTextID[2];


	if(XMSYS_system_update_get_version() == 0xFFFFFFFF)
	{
		// �Զ���װ�������汾, "��ȷ������ģʽ"
		// ִ��"ϵͳ����"����
		AP_OpenSystemUpdateView (0);
		return;
	}

	dwButtonNormalTextID[0] = AP_ID_SYSTEMUPDATE_YES;
	dwButtonNormalTextID[1] = AP_ID_SYSTEMUPDATE_CANCEL;
	dwButtonPressedTextID[0] = AP_ID_SYSTEMUPDATE_YES;
	dwButtonPressedTextID[1] = AP_ID_SYSTEMUPDATE_CANCEL;
	XM_OpenAlertView (
					AP_ID_SYSTEMUPDATE_TITLE,				// ��Ϣ�ı���ԴID
					0,												// ͼƬ��Ϣ��ԴID
					2,												// ��ť����
					dwButtonNormalTextID,					// ��ť������ԴID
					dwButtonPressedTextID,					// ��ť����������ԴID
					APP_ALERT_BKGCOLOR,						// Alert�Ӵ�����ɫ
					30.0,											// ����30�����Զ��ر�, 0 ��ʾ��ֹ�Զ��ر�
					APP_ALERT_BKGALPHA,						// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
					SystemUpdateDialogCallBack,			// �����ص�����
					NULL,											// �û��ص�����˽�в���
					XM_VIEW_ALIGN_BOTTOM,					// ��ͼ����ʾ����������Ϣ
					//XM_VIEW_ALIGN_CENTRE,
					XM_ALERTVIEW_OPTION_ENABLE_CALLBACK	// ʹ�ܰ����ص���������, ALERTVIEW��ͼ�Ŀ���ѡ��
					);
}

// #define	XM_CARD			0x10	// SD������¼�
										// lp����Ϊ0
										// wp = 0, SD���γ��¼�
										// wp = 1, SD������(д����)
										// wp = 2, SD������(��д����)

VOID DesktopOnSystemEvent (XMMSG *msg)
{
	XM_printf(">>>>>>>>DesktopOnSystemEvent, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);

	XM_DefaultSystemEventProc(XMHWND_HANDLE(Desktop), msg);
}


// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (Desktop)
	XM_ON_MESSAGE (XM_KEYDOWN, DesktopOnKeyDown)
	XM_ON_MESSAGE (XMKEY_REPEAT, DesktopOnKeyRepeatDown)
	XM_ON_MESSAGE (XMKEY_LONGTIME, DesktopOnKeyLongTimeDown)
	XM_ON_MESSAGE (XM_ENTER, DesktopOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, DesktopOnLeave)
	XM_ON_MESSAGE (XM_TIMER, DesktopOnTimer)
	XM_ON_MESSAGE (XM_SYSTEMEVENT, DesktopOnSystemEvent)
XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, Desktop, 1, 0, 0, 0, HWND_VIEW)


