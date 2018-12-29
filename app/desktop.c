//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	SongXing
//
//	File name: desktop.c
//	  行车记录状态主界面
//	  桌面是一个隐藏的系统控制窗口，负责事件的缺省处理，不支持实际显示
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
// 检查物理SD卡是否插入
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

// 开机自检
static VOID DesktopOnSystemCheck (void);
static void OpenSystemUpdateDialog (void);	// 打开系统升级提示对话框

#define	OSD_AUTO_HIDE_TIMEOUT			10000

#define DEMO_MODE			0			// 演示模式(无卡插入、卡写保护或卡损坏)
#define WAITING_MODE		1			// 等待模式(延时纪录等待)
#define RECORD_MODE		2			// 记录模式

// 系统开机自检过程定义
enum {
	SELFCHECK_STEP_BATTERY = 0,	// 电池状况检查
	SELFCHECK_STEP_TIME,				// 系统时间设置检查
	SELFCHECK_STEP_CARD,				// 卡及文件系统检测
	SELFCHECK_STEP_VIDEO,			// 视频项记录相关检查
	SELFCHECK_STEP_BYPASS,			// 系统开机自检已完成
	SELFCHECK_STEP_COUNT
};


/*
系统预设条件, 首次启动；回放模式为仅主摄像头, 缺省为记录模式
*/

int	iPlaybackMode;		// 在显示屏显示哪一些摄像头的画面
int	iRecordMode;		// 当前的行车记录方式:  Demo 、Record、WAITING
int CurrentPhotoMode =0;
static int SelfCheckStep = SELFCHECK_STEP_BATTERY;
static unsigned int ticket_to_card_verify_warning;

#define	XM_USER_SYSCHECKING	(XM_USER+0)
#define BATTERY_LOW_POWER_OFF_TIMEOUT   (5 * 1000)

// 标记卡重新检查
// 当重新返回到桌面模式时，重新检查
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

	// 设置初始ICON状态
	XM_SetFmlDeviceCap (DEVCAP_VIDEO_REC, DEVCAP_VIDEO_REC_STOP);
}

#include "gpio.h"
#include "ark1960.h"
u8_t Auto_Switch_3s = 0;
extern BOOL Close_Brighness;
//extern BOOL NO_Signed_Fail;
BOOL Auto_Menu = FALSE; //进入倒车状态,返回Desktop界面,然后按按键还是的有反应
u8_t Data_PowerOff_Count = 0;

VOID DesktopOnEnter (XMMSG *msg)
{
    CurrentPhotoMode = 0;
    //NO_Signed_Fail = TRUE;
	if(SelfCheckStep == SELFCHECK_STEP_BATTERY)
	{
		// 开机复位
		// 设置初始ICON状态
		if(ACC_DET_First) //防止倒车返回重置录像状态
		{
		    XM_SetFmlDeviceCap (DEVCAP_VIDEO_REC, DEVCAP_VIDEO_REC_STOP);
        }
	}
    AP_SetMenuItem(APPMENUITEM_LCD, AP_GetMenuItem(APPMENUITEM_LCD));
    #if 0
	if (msg->wp == 0)
	{
		// 桌面创建
		DesktopOnSystemCheck ();
	}
	else
	{
		// 桌面重新进入
		DesktopOnSystemCheck ();
	}
    #endif
//
	//加载主页资源到链表
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
        //APP_SetPowerOff_Restore(1);//自动关机后,是为关机模式
		XM_printf(">>>>>>APP_GetMemory():%d\r\n", APP_GetMemory());
		XM_printf(">>>>>>AP_GetLogo():%d\r\n", AP_GetLogo());
		
        //if(APP_GetMemory()) { //关机模式
        //    XM_KeyEventProc (VK_AP_POWER,XM_KEYDOWN); //自动切换
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
            //正常开机,保存下来.
            #if 0
            if(AP_GetBright_Switch() == 0){
				APP_GetPowerOn_Memory(0); //设置为关机状态.需要缓存当前状态
			}else {
            	APP_SetPowerOn_Memory(Close_Brighness);//设置当前记忆的模式
			}
			#endif
			APP_SetPowerOn_Memory(Close_Brighness);//设置当前记忆的模式
        }

		APP_SetPowerOff_Restore(1);
		APP_SaveMenuData ();
    }
    XM_voice_prompts_insert_voice (XM_VOICE_ID_STARTUP);
    Auto_Switch_3s = 0;// 初始化3s轮换参数
    Auto_Menu = FALSE;
    Data_PowerOff_Count = 0; //返回界面开始计时
    XM_SetTimer (XMTIMER_DESKTOPVIEW, 500); // 500
}

// 开机自检
static VOID DesktopOnSystemCheck (void)
{
	DWORD	dwState;

	// 检查是否有自动升级版本(内部测试使用)需要安装, 若存在, 不检查时间设置
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
		// 电池检测状态
		// 电池检测OK, 切换到RTC时间检查状态
		SelfCheckStep = SELFCHECK_STEP_TIME;
	}

	if(SelfCheckStep == SELFCHECK_STEP_TIME)
	{
		// 检查时间设置
		dwState = XM_GetFmlDeviceCap(DEVCAP_TIMESETTING);
		if(dwState == DEVCAP_TIMESETTING_NG)
		{
			// 设置一个缺省时间, 否则无法开始记录视频.
			// 视频保存需要RTC时间已设置
			XMSYSTEMTIME local_time;
			local_time.wYear = 2017;
			local_time.bMonth = 1;
			local_time.bDay = 1;
			local_time.bHour = 0;
			local_time.bMinute = 0;
			local_time.bSecond = 0;
			local_time.wMilliSecond = 0;
			XM_SetLocalTime (&local_time);

			// 未设置，进入初始设置界面
			//XM_PushWindow (XMHWND_HANDLE(InitSettingView));
			//return;
		}

		// 时间检测PASS, 切换到卡检测
		SelfCheckStep = SELFCHECK_STEP_CARD;
	}

	if(SelfCheckStep == SELFCHECK_STEP_CARD)
	{
		// 卡检测模式. 检查卡是否正确的插入
		if(XM_GetFmlDeviceCap(DEVCAP_SDCARDSTATE) != DEVCAP_SDCARDSTATE_INSERT)
		{
			// 卡没有正确的插入, 切换到演示模式
			goto demo_mode;
		}

		// 卡已正确插入, 切换到录像检测模式
		SelfCheckStep = SELFCHECK_STEP_VIDEO;
	}

	if(SelfCheckStep == SELFCHECK_STEP_VIDEO)
	{
		// 录像检测模式, 检查系统是否可以满足视频录像的条件
		// 检查视频项基本服务是否已开启
		if(!XM_VideoItemIsBasicServiceOpened())
		{
			// 视频项基本服务打开失败
			// 切换到演示模式
			goto demo_mode;
		}
		SelfCheckStep = SELFCHECK_STEP_BYPASS;
	}

	if(SelfCheckStep != SELFCHECK_STEP_BYPASS)
		return;

#if CZS_USB_01
	// 开机录像
#else
	// 检查延时录像是否使能
	if((XM_GetTickCount() - AppGetStartupTicket()) < (AP_GetRecordDelayTime () * 1000))
	{
		// 录像延时
		XM_SetTimer (XMTIMER_DESKTOPVIEW, 100);	// 创建100ms的定时器
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
			// 启动定时器，100ms后继续启动记录模式
			XM_SetTimer (XMTIMER_DESKTOPVIEW, 100);
			iRecordMode = WAITING_MODE;
		}
	}

	//return;
	// 演示模式
demo_mode:
	XM_KillTimer (XMTIMER_DESKTOPVIEW);
	iRecordMode = DEMO_MODE;
	XMSYS_H264CodecSetForcedNonRecordingMode(1); //无SD卡上电无法在"停止录像的情况"下进入设置菜单
	XM_SetFmlDeviceCap (DEVCAP_VIDEO_REC, DEVCAP_VIDEO_REC_STOP);
	XM_printf(">>>>>DesktopOnSystemCheck start recorder..........\r\n");

	XMSYS_H264CodecRecorderStart();	//  开启录像通道
}


/*
OnLeave
	进入其他界面，需要开启UI。此时系统需依据回放是否停止，决定是否将回放与UI合成
*/
VOID DesktopOnLeave (XMMSG *msg)
{
	// 删除定时器
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
OnTimer：延时启动计数器和状态闪烁

	如果延时记录定时结束
		关闭UI
		开始记录用户安装的所有摄像头画面。（连接中断的摄像头以固定模式的画面记录）

	依据系统设置，计算图标需要占用面积，设置保留相应的显示空间，各种状态图标按固定频率闪烁（副摄像头连接状态，电池状态），生成位图，显示到保留位置

	如果是Demo模式，按固定频率开启UI，显示演示状态，提醒用户
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
    //开启倒车标志
    if((Guide_Camera_Show() ||ACC_Select_BiaoCHi_Show)&& ACC_DET_First&& Close_Audio_Sound)
    {
        ACC_DET_First = FALSE;
        XM_PushWindow (XMHWND_HANDLE(CarBackLineView));
    }else{
        ACC_DET_First = TRUE;
    }

	
    //3s 自动循环切换问题 当关闭背光的时候,不在切换
    if(APP_GetAuto_Switch() && !Close_Brighness&&Close_Audio_Sound)
    {
        Auto_Switch_3s ++;
        if(Auto_Switch_3s > 12)
        {
            Auto_Switch_3s = 0;
            XM_PostMessage (XM_KEYDOWN, VK_AP_AV, 0); //自动切换
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
	}else if((Close_Reverse_Back == TRUE)&& !AHD_Guide_Start()&&(Data_PowerOff_Count > 5)){ //倒车结束后重新处于关机状态
	    Close_Reverse_Back = FALSE;
        Data_PowerOff_Count = 0;
	    //XM_KeyEventProc (VK_AP_POWER,XM_KEYDOWN);
    }
    #endif

	// 100ms为单位
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
			{//非录像模式
			    // 无法切换到录像模式，弹出信息窗口，提示原因
				unsigned int card_state = XM_GetFmlDeviceCap(DEVCAP_SDCARDSTATE);
				if(card_state == DEVCAP_SDCARDSTATE_INSERT)
				{
			        XMSYS_H264CodecSetForcedNonRecordingMode(0);//设置成录像模式,强制非录像模式到录像模式转变,启动录像
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


// 文件系统无法访问错误回调函数
static void fs_access_error_alert_cb (void *userPrivate, unsigned int uKeyPressed)
{
	if(uKeyPressed == VK_AP_MENU)		// “格式化”按键
	{
		// 格式化操作
		AP_OpenFormatView (0);
	}
	else					// “取消”按键
	{
		// 返回到调用者窗口
		XM_PullWindow (0);
	}
}

// 文件系统低性能回调函数
static void fs_low_performance_alert_cb (void *userPrivate, unsigned int uKeyPressed)
{
	if(uKeyPressed == VK_AP_MENU)		// “格式化”按键
	{
		// 格式化操作
		AP_OpenFormatView (0);
	}
	else					// “取消”按键
	{
		// 返回到调用者窗口
		XM_PullWindow (0);
	}
}

// 文件系统读写校验错误回调函数
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
        case VK_AP_UP:		// 紧急录像键
        
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

		case VK_AP_DOWN:		// 一键拍照
            if(AP_GetMidle_RED_Line()) {
                Red_Location = Red_Location - 1;
                AP_SetRed_Location(Red_Location);
                AP_SaveMenuData (&AppMenuData);
            }
            else {
                if(ACC_Select == 4) //第四通道不要调试声音
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
// 按键音
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
		case VK_AP_MENU:		// 菜单键
            if(XM_GetFmlDeviceCap(DEVCAP_VIDEO_REC) == DEVCAP_VIDEO_REC_STOP)
            {
				XM_PushWindow (XMHWND_HANDLE(SettingView));
			}
			
			//AP_OpenFormatView(0);
		    if(AHD_Guide_Start())
				Auto_Menu = TRUE;
			break;

		case VK_AP_MODE:		// 切换到录像回放模式
			XM_printf(">>>>>>>VK_AP_MODE, iRecordMode:%d\r\n", iRecordMode);
			if(iRecordMode == DEMO_MODE)
			{
				// 无法切换到视频预览模式，弹出信息窗口，提示原因
				unsigned int card_state = XM_GetFmlDeviceCap(DEVCAP_SDCARDSTATE);
				if(card_state == DEVCAP_SDCARDSTATE_UNPLUG)
				{
					// 卡已拔出
					XM_OpenAlertView (
						AP_ID_CARD_MESSAGE_CARD_WITHDRAW,	// 信息文本资源ID
						AP_ID_CARD_ICON_SDCARD,			// 图片信息资源ID
						0,
						0,												// 按钮文字资源ID
						0,												// 按钮按下文字资源ID
						APP_ALERT_BKGCOLOR,
						10.0,											// 10秒钟自动关闭
						APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
						NULL,											// 按键回调函数
						NULL,											// 用户回调函数私有参数
						XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
						//XM_VIEW_ALIGN_CENTRE,
						0												// ALERTVIEW视图的控制选项
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

					// 卡文件系统错误
					XM_OpenAlertView (
						AP_ID_CARD_MESSAGE_CARD_FSERROR,		// 信息文本资源ID
						AP_ID_CARD_ICON_SDCARD,												// 图片信息资源ID
						2,												// 按钮个数
						dwButtonNormalTextID,					// 按钮文字资源ID
						dwButtonPressedTextID,					// 按钮按下文字资源ID
						APP_ALERT_BKGCOLOR,
						10.0,											// 10秒钟自动关闭
						APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
						fs_access_error_alert_cb,							// 按键回调函数
						NULL,											// 用户回调函数私有参数
						XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
						//XM_VIEW_ALIGN_CENTRE,
						XM_ALERTVIEW_OPTION_ENABLE_CALLBACK	// ALERTVIEW视图的控制选项
						);
				}
				else if(card_state == DEVCAP_SDCARDSTATE_INVALID)
				{
					// 卡无效(卡无法识别)
					XM_OpenAlertView (
						AP_ID_CARD_MESSAGE_CARD_INVALID,	// 信息文本资源ID
						AP_ID_CARD_ICON_SDCARD,			// 图片信息资源ID
						0,
						0,												// 按钮文字资源ID
						0,												// 按钮按下文字资源ID
						APP_ALERT_BKGCOLOR,
						10.0,											// 10秒钟自动关闭
						APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
						NULL,											// 按键回调函数
						NULL,											// 用户回调函数私有参数
						XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
						//XM_VIEW_ALIGN_CENTRE,
						0												// ALERTVIEW视图的控制选项
						);
				}
				else
				{
					#if 1 //必须在停止录像的条件下才可以进入设置菜单
					if(!XMSYS_H264CodecGetForcedNonRecordingMode())
					{
				    	// 停止录像
						XM_OpenAlertView (
							AP_ID_VIDEO_MESSAGE_VIDEO_STOP,			// 信息文本资源ID
							AP_ID_VIDEO_ICON_VIDEOSTOP,				// 图片信息资源ID
							0,
							0,										// 按钮文字资源ID
							0,										// 按钮按下文字资源ID
							APP_ALERT_BKGCOLOR,
							(float)1.2,		// 自动关闭时间
							APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
							NULL,									// 按键回调函数
							NULL,									// 用户回调函数私有参数
							XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
							//XM_VIEW_ALIGN_CENTRE,
							0										// ALERTVIEW视图的控制选项
							);
					   return;
					}
					#endif

					// 检查视频项列表是否正在扫描中.
					// 若正在扫描, 弹出一POP视窗提示用户"正在扫描视频文件,请稍等"
					if(!XM_VideoItemIsBasicServiceOpened() && !XM_VideoItemIsServiceOpened())
					{
						XM_OpenAlertView (
							AP_ID_VIDEO_VIEW_SCAN_VIDEO_FILE,	// 信息文本资源ID
							AP_ID_CARD_ICON_SDCARD,			// 图片信息资源ID
							0,
							0,												// 按钮文字资源ID
							0,												// 按钮按下文字资源ID
							APP_ALERT_BKGCOLOR,
							60.0,											// 60秒钟自动关闭
							APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
							NULL,											// 按键回调函数
							NULL,											// 用户回调函数私有参数
							XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
							//XM_VIEW_ALIGN_CENTRE,
							0												// ALERTVIEW视图的控制选项
							);

					}
					else
					{
						// 卡插入(读写模式、写保护模式)
						APPMarkCardChecking (1);		// 终止视频通道
						XM_PushWindow (XMHWND_HANDLE(VideoListView));
					}
				}
			}
			else
			{
				#if 1  //必须在停止录像的条件下才可以进入设置菜单
				if(!XMSYS_H264CodecGetForcedNonRecordingMode())
				{
			    	// 停止录像
					XM_OpenAlertView (
						AP_ID_VIDEO_MESSAGE_VIDEO_STOP,			// 信息文本资源ID
						AP_ID_VIDEO_ICON_VIDEOSTOP,				// 图片信息资源ID
						0,
						0,										// 按钮文字资源ID
						0,										// 按钮按下文字资源ID
						APP_ALERT_BKGCOLOR,
						(float)1.2,		// 自动关闭时间
						APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
						NULL,									// 按键回调函数
						NULL,									// 用户回调函数私有参数
						XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
						//XM_VIEW_ALIGN_CENTRE,
						0										// ALERTVIEW视图的控制选项
						);
				   return;
				}
				#endif

				// 检查视频项列表是否正在扫描中.
				// 若正在扫描, 弹出一POP视窗提示用户"正在扫描视频文件,请稍等"
				if(XM_VideoItemIsBasicServiceOpened() && !XM_VideoItemIsServiceOpened())
				{
					XM_OpenAlertView (
							AP_ID_VIDEO_VIEW_SCAN_VIDEO_FILE,	// 信息文本资源ID
							AP_ID_CARD_ICON_SDCARD,			// 图片信息资源ID
							0,
							0,												// 按钮文字资源ID
							0,												// 按钮按下文字资源ID
							APP_ALERT_BKGCOLOR,
							60.0,											// 60秒钟自动关闭
							APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
							NULL,											// 按键回调函数
							NULL,											// 用户回调函数私有参数
							XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
							//XM_VIEW_ALIGN_CENTRE,
							0												// ALERTVIEW视图的控制选项
							);

				}
				else
				{
					APPMarkCardChecking (1);		// 终止视频通道
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
		case VK_AP_FONT_BACK_SWITCH:	// 画面切换键
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
			   if((ITU656_in==1)&&(Reversing==0))  //没有倒车信号才容许切换
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
			{//非录像模式
			    // 无法切换到录像模式，弹出信息窗口，提示原因
				unsigned int card_state = XM_GetFmlDeviceCap(DEVCAP_SDCARDSTATE);
				if(card_state == DEVCAP_SDCARDSTATE_UNPLUG)
				{
					// 卡已拔出
					XM_OpenAlertView(
						AP_ID_CARD_MESSAGE_CARD_WITHDRAW,			// 信息文本资源ID
						AP_ID_CARD_ICON_SDCARD,						// 图片信息资源ID
						0,
						0,											// 按钮文字资源ID
						0,											// 按钮按下文字资源ID
						APP_ALERT_BKGCOLOR,
						2.0,										// 10秒钟自动关闭
						APP_ALERT_BKGALPHA,							// 信息视图的视窗Alpha因子，0.0 ~ 1.0
						NULL,										// 按键回调函数
						NULL,										// 用户回调函数私有参数
						XM_VIEW_ALIGN_BOTTOM,						// 视图的显示对齐设置信息
						0											// ALERTVIEW视图的控制选项
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

					// 卡文件系统错误
					XM_OpenAlertView (
						AP_ID_CARD_MESSAGE_CARD_FSERROR,				// 信息文本资源ID
						AP_ID_CARD_ICON_SDCARD,							// 图片信息资源ID
						2,												// 按钮个数
						dwButtonNormalTextID,							// 按钮文字资源ID
						dwButtonPressedTextID,							// 按钮按下文字资源ID
						APP_ALERT_BKGCOLOR,
						2.0,											// 10秒钟自动关闭
						APP_ALERT_BKGALPHA,								// 信息视图的视窗Alpha因子，0.0 ~ 1.0
						fs_access_error_alert_cb,					// 按键回调函数
						NULL,											// 用户回调函数私有参数
						XM_VIEW_ALIGN_BOTTOM,							// 视图的显示对齐设置信息
						XM_ALERTVIEW_OPTION_ENABLE_CALLBACK				// ALERTVIEW视图的控制选项
						);
				}
				else if(card_state == DEVCAP_SDCARDSTATE_INVALID)
				{
					// 卡无效(卡无法识别)
					XM_OpenAlertView (
						AP_ID_CARD_MESSAGE_CARD_INVALID,				// 信息文本资源ID
						AP_ID_CARD_ICON_SDCARD,							// 图片信息资源ID
						0,
						0,												// 按钮文字资源ID
						0,												// 按钮按下文字资源ID
						APP_ALERT_BKGCOLOR,
						2.0,											// 10秒钟自动关闭
						APP_ALERT_BKGALPHA,								// 信息视图的视窗Alpha因子，0.0 ~ 1.0
						NULL,											// 按键回调函数
						NULL,											// 用户回调函数私有参数
						XM_VIEW_ALIGN_BOTTOM,							// 视图的显示对齐设置信息
						0												// ALERTVIEW视图的控制选项
						);
				}
			    else
			    {
			        XMSYS_H264CodecSetForcedNonRecordingMode(0);//设置成录像模式,强制非录像模式到录像模式转变,启动录像
					XM_SetFmlDeviceCap(DEVCAP_VIDEO_REC, DEVCAP_VIDEO_REC_START);
			    }
			}
		  	else
			{
			    XMSYS_H264CodecSetForcedNonRecordingMode(1);//设置成强制非录像模式
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
		case VK_AP_UP://+键
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
                if(APP_GetVOl_OnOff()) //声音关闭的时候,不需要调试音量
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
		//case VK_AP_UP:		// 切换到录像回放模式
		case VK_AP_SWITCH:      //改回这个,便于vcom长按测试
			//	SetGPIOPadData (GPIO30, euDataLow);//guan anjian shengyin 蜂鸣器口??
			if(iRecordMode == DEMO_MODE)
			{
				// 无法切换到视频预览模式，弹出信息窗口，提示原因
				unsigned int card_state = XM_GetFmlDeviceCap(DEVCAP_SDCARDSTATE);
				if(card_state == DEVCAP_SDCARDSTATE_UNPLUG)
				{
					// 卡已拔出
					XM_OpenAlertView (
						AP_ID_CARD_MESSAGE_CARD_WITHDRAW,	// 信息文本资源ID
						AP_ID_CARD_ICON_SDCARD,			// 图片信息资源ID
						0,
						0,												// 按钮文字资源ID
						0,												// 按钮按下文字资源ID
						APP_ALERT_BKGCOLOR,
						10.0,											// 10秒钟自动关闭
						APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
						NULL,											// 按键回调函数
						NULL,											// 用户回调函数私有参数
						XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
						//XM_VIEW_ALIGN_CENTRE,
						0												// ALERTVIEW视图的控制选项
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

					// 卡文件系统错误
					XM_OpenAlertView (
						AP_ID_CARD_MESSAGE_CARD_FSERROR,		// 信息文本资源ID
						AP_ID_CARD_ICON_SDCARD,												// 图片信息资源ID
						2,												// 按钮个数
						dwButtonNormalTextID,					// 按钮文字资源ID
						dwButtonPressedTextID,					// 按钮按下文字资源ID
						APP_ALERT_BKGCOLOR,
						10.0,											// 10秒钟自动关闭
						APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
						fs_access_error_alert_cb,							// 按键回调函数
						NULL,											// 用户回调函数私有参数
						XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
						//XM_VIEW_ALIGN_CENTRE,
						XM_ALERTVIEW_OPTION_ENABLE_CALLBACK	// ALERTVIEW视图的控制选项
						);
				}
				else if(card_state == DEVCAP_SDCARDSTATE_INVALID)
				{
					// 卡无效(卡无法识别)
					XM_OpenAlertView (
						AP_ID_CARD_MESSAGE_CARD_INVALID,	// 信息文本资源ID
						AP_ID_CARD_ICON_SDCARD,			// 图片信息资源ID
						0,
						0,												// 按钮文字资源ID
						0,												// 按钮按下文字资源ID
						APP_ALERT_BKGCOLOR,
						10.0,											// 10秒钟自动关闭
						APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
						NULL,											// 按键回调函数
						NULL,											// 用户回调函数私有参数
						XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
						//XM_VIEW_ALIGN_CENTRE,
						0												// ALERTVIEW视图的控制选项
						);
				}
				else
				{
					#if 1 //必须在停止录像的条件下才可以进入设置菜单
					if(!XMSYS_H264CodecGetForcedNonRecordingMode())
					{
				    	// 停止录像
						XM_OpenAlertView (
							AP_ID_VIDEO_MESSAGE_VIDEO_STOP,			// 信息文本资源ID
							AP_ID_VIDEO_ICON_VIDEOSTOP,				// 图片信息资源ID
							0,
							0,										// 按钮文字资源ID
							0,										// 按钮按下文字资源ID
							APP_ALERT_BKGCOLOR,
							(float)1.2,		// 自动关闭时间
							APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
							NULL,									// 按键回调函数
							NULL,									// 用户回调函数私有参数
							XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
							//XM_VIEW_ALIGN_CENTRE,
							0										// ALERTVIEW视图的控制选项
							);
					   return;
					}
					#endif

					// 检查视频项列表是否正在扫描中.
					// 若正在扫描, 弹出一POP视窗提示用户"正在扫描视频文件,请稍等"
					if(!XM_VideoItemIsBasicServiceOpened() && !XM_VideoItemIsServiceOpened())
					{
						XM_OpenAlertView (
							AP_ID_VIDEO_VIEW_SCAN_VIDEO_FILE,	// 信息文本资源ID
							AP_ID_CARD_ICON_SDCARD,			// 图片信息资源ID
							0,
							0,												// 按钮文字资源ID
							0,												// 按钮按下文字资源ID
							APP_ALERT_BKGCOLOR,
							60.0,											// 60秒钟自动关闭
							APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
							NULL,											// 按键回调函数
							NULL,											// 用户回调函数私有参数
							XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
							//XM_VIEW_ALIGN_CENTRE,
							0												// ALERTVIEW视图的控制选项
							);

					}
					else
					{
						// 卡插入(读写模式、写保护模式)
						APPMarkCardChecking (1);		// 终止视频通道
						XM_PushWindow (XMHWND_HANDLE(VideoListView));
					}
				}
			}
			else
			{
				#if 1  //必须在停止录像的条件下才可以进入设置菜单
				if(!XMSYS_H264CodecGetForcedNonRecordingMode())
				{
			    	// 停止录像
					XM_OpenAlertView (
						AP_ID_VIDEO_MESSAGE_VIDEO_STOP,			// 信息文本资源ID
						AP_ID_VIDEO_ICON_VIDEOSTOP,				// 图片信息资源ID
						0,
						0,										// 按钮文字资源ID
						0,										// 按钮按下文字资源ID
						APP_ALERT_BKGCOLOR,
						(float)1.2,		// 自动关闭时间
						APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
						NULL,									// 按键回调函数
						NULL,									// 用户回调函数私有参数
						XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
						//XM_VIEW_ALIGN_CENTRE,
						0										// ALERTVIEW视图的控制选项
						);
				   return;
				}
				#endif

				// 检查视频项列表是否正在扫描中.
				// 若正在扫描, 弹出一POP视窗提示用户"正在扫描视频文件,请稍等"
				if(XM_VideoItemIsBasicServiceOpened() && !XM_VideoItemIsServiceOpened())
				{
					XM_OpenAlertView(
							AP_ID_VIDEO_VIEW_SCAN_VIDEO_FILE,	// 信息文本资源ID
							AP_ID_CARD_ICON_SDCARD,			// 图片信息资源ID
							0,
							0,												// 按钮文字资源ID
							0,												// 按钮按下文字资源ID
							APP_ALERT_BKGCOLOR,
							60.0,											// 60秒钟自动关闭
							APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
							NULL,											// 按键回调函数
							NULL,											// 用户回调函数私有参数
							XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
							//XM_VIEW_ALIGN_CENTRE,
							0												// ALERTVIEW视图的控制选项
							);
				}
				else
				{
					APPMarkCardChecking (1);		// 终止视频通道
					if(XM_GetFmlDeviceCap(DEVCAP_SDCARDSTATE) == DEVCAP_SDCARDSTATE_INSERT)
					{
						XM_PushWindow(XMHWND_HANDLE(VideoListView));//视频6宫格
						//XM_PushWindow(XMHWND_HANDLE(AlbumListView));//图片6宫格
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
	    // 无法切换到录像模式，弹出信息窗口，提示原因
		unsigned int card_state = XM_GetFmlDeviceCap(DEVCAP_SDCARDSTATE);
		if(card_state == DEVCAP_SDCARDSTATE_UNPLUG)
		{
			// 卡已拔出
			XM_OpenAlertView (
				AP_ID_CARD_MESSAGE_CARD_WITHDRAW,			// 信息文本资源ID
				AP_ID_CARD_ICON_SDCARD,						// 图片信息资源ID
				0,
				0,											// 按钮文字资源ID
				0,											// 按钮按下文字资源ID
				APP_ALERT_BKGCOLOR,
				10.0,										// 10秒钟自动关闭
				APP_ALERT_BKGALPHA,							// 信息视图的视窗Alpha因子，0.0 ~ 1.0
				NULL,										// 按键回调函数
				NULL,										// 用户回调函数私有参数
				XM_VIEW_ALIGN_BOTTOM,						// 视图的显示对齐设置信息
				0											// ALERTVIEW视图的控制选项
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

			// 卡文件系统错误
			XM_OpenAlertView (
				AP_ID_CARD_MESSAGE_CARD_FSERROR,				// 信息文本资源ID
				AP_ID_CARD_ICON_SDCARD,							// 图片信息资源ID
				2,												// 按钮个数
				dwButtonNormalTextID,							// 按钮文字资源ID
				dwButtonPressedTextID,							// 按钮按下文字资源ID
				APP_ALERT_BKGCOLOR,
				10.0,											// 10秒钟自动关闭
				APP_ALERT_BKGALPHA,								// 信息视图的视窗Alpha因子，0.0 ~ 1.0
				fs_access_error_alert_cb,					// 按键回调函数
				NULL,											// 用户回调函数私有参数
				XM_VIEW_ALIGN_BOTTOM,							// 视图的显示对齐设置信息
				XM_ALERTVIEW_OPTION_ENABLE_CALLBACK				// ALERTVIEW视图的控制选项
				);
		}
		else if(card_state == DEVCAP_SDCARDSTATE_INVALID)
		{
			// 卡无效(卡无法识别)
			XM_OpenAlertView (
				AP_ID_CARD_MESSAGE_CARD_INVALID,				// 信息文本资源ID
				AP_ID_CARD_ICON_SDCARD,							// 图片信息资源ID
				0,
				0,												// 按钮文字资源ID
				0,												// 按钮按下文字资源ID
				APP_ALERT_BKGCOLOR,
				10.0,											// 10秒钟自动关闭
				APP_ALERT_BKGALPHA,								// 信息视图的视窗Alpha因子，0.0 ~ 1.0
				NULL,											// 按键回调函数
				NULL,											// 用户回调函数私有参数
				XM_VIEW_ALIGN_BOTTOM,							// 视图的显示对齐设置信息
				0												// ALERTVIEW视图的控制选项
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
OnSystemEvent：各种系统事件的处理
	文件系统异常（需格式化的异常）
	可记录时间低
	备份电池电压低等在启动过程处理，行车过程中不再提示。

	卡插入，进行与OnEnter一样的检查，提示记录时间，录音状态，因为摄像头连接状态用户已经知道，不提示是否部分摄像头是否连接

	卡拔出：演示状态下，不作任何处理。行车记录状态下，关闭行车记录，进入提示界面，等待用户确认。退出回放界面时，会由OnEnter再次判断该进入那一个状态

	副摄像头连接中断：语音提示，记录状态？

	文件系统异常：行车记录状态下，关闭行车记录，重新进入OnEnter，进行相应处理

	时长不足：语音提示
*/

VOID XM_DefaultSystemEventProc (HANDLE hWnd, XMMSG *msg)
{
	//HANDLE hDesktop = XM_GetDesktop ();
	XM_BreakSystemEventDefaultProcess (msg);
	switch(msg->wp)
	{
		// ------------------------------------
		//
		// *** 卡事件处理 ***
		//
		// ------------------------------------
		case SYSTEM_EVENT_CARD_DETECT:			// SD卡准备中(SD卡检测中)
			// 卡检测中(检测到卡已插入，正在识别中)，提示用户
#if CZS_USB_01

#else
			XM_OpenAlertView (
							AP_ID_CARD_MESSAGE_CARD_DETECT,	// 信息文本资源ID
							AP_ID_CARD_ICON_SDCARD,			// 图片信息资源ID
							0,
							0,												// 按钮文字资源ID
							0,												// 按钮按下文字资源ID
							APP_ALERT_BKGCOLOR,
							2.0,											// 5秒钟自动关闭
							APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
							NULL,											// 按键回调函数
							NULL,											// 用户回调函数私有参数
							XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
							//XM_VIEW_ALIGN_CENTRE,
							0												// ALERTVIEW视图的控制选项
							);
#endif
			break;

		case SYSTEM_EVENT_CARD_UNPLUG:			// SD卡拔出事件
			XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_CARD_UNPLUG................\r\n");
			// 标记卡重新检查
			XMSYS_UvcSocketReportSDCardSystemEvent(SYSTEM_EVENT_CARD_UNPLUG);
			APPMarkCardChecking(0);
			XM_AppInitStartupTicket();  //解决卡拔出，计时时间重新计
			XMSYS_H264CodecSetForcedNonRecordingMode(1);//设置成强制非录像模式
			#if 0
			/*
			if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
				XM_voice_prompts_insert_voice (XM_VOICE_ID_ALARM);
			*/
#if CZS_USB_01
#else
			XM_OpenAlertView (
					AP_ID_CARD_MESSAGE_CARD_WITHDRAW,	// 信息文本资源ID
					AP_ID_CARD_ICON_SDCARD,			// 图片信息资源ID
					0,
					0,												// 按钮文字资源ID
					0,												// 按钮按下文字资源ID
					APP_ALERT_BKGCOLOR,
					2.0,											// 5秒钟自动关闭
					APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
					NULL,											// 按键回调函数
					NULL,											// 用户回调函数私有参数
					XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
					//XM_VIEW_ALIGN_CENTRE,
					0												// ALERTVIEW视图的控制选项
					);
#endif
			#endif
			break;

		case SYSTEM_EVENT_CARD_FS_ERROR:					// 文件系统异常事件(无法识别的数据卡)
			XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_CARD_FS_ERROR................\r\n");
			#if 0
			XMSYS_UvcSocketReportSDCardSystemEvent (SYSTEM_EVENT_CARD_FS_ERROR);
			APPMarkCardChecking	(0);
			/*
			// 标记卡重新检查
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
					// 卡文件系统错误
					XM_OpenAlertView (
						AP_ID_CARD_MESSAGE_CARD_FSERROR,		// 信息文本资源ID
						AP_NULLID, // AP_ID_CARD_ICON_SDCARD,												// 图片信息资源ID
						2,												// 按钮个数
						dwButtonNormalTextID,					// 按钮文字资源ID
						dwButtonPressedTextID,					// 按钮按下文字资源ID
						APP_ALERT_BKGCOLOR,
						10.0,											// 30秒钟自动关闭
						APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
						fs_access_error_alert_cb,							// 按键回调函数
						NULL,											// 用户回调函数私有参数
						XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
						//XM_VIEW_ALIGN_CENTRE,
						XM_ALERTVIEW_OPTION_ENABLE_CALLBACK	// ALERTVIEW视图的控制选项
						);
			}
#endif
			#endif
			break;

		case SYSTEM_EVENT_CARD_VERIFY_ERROR:	// 卡读写校验失败(卡物理性损坏或者卡文件系统异常)
			XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_CARD_VERIFY_ERROR................\r\n");
			// 检查卡是否插入, 防止假报警
			if(!XMSYS_check_sd_card_exist())
				break;
			XMSYS_UvcSocketReportSDCardSystemEvent (SYSTEM_EVENT_CARD_VERIFY_ERROR);

			if(XM_GetTickCount() < ticket_to_card_verify_warning)
				break;

			ticket_to_card_verify_warning = XM_GetTickCount() + 10*1000;

			// 继续录像
			if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
				XM_voice_prompts_insert_voice (XM_VOICE_ID_ALARM );
#if CZS_USB_01
#else
			// 切换到卡信息视窗
			XM_OpenAlertView (
					AP_ID_CARD_MESSAGE_CARD_VERIFYFAILED,	// 信息文本资源ID
					AP_ID_CARD_ICON_SDCARD,												// 图片信息资源ID
					0,												// 按钮个数
					0,												// 按钮文字资源ID
					0,												// 按钮按下文字资源ID
					APP_ALERT_BKGCOLOR,
					10.0,											// 10秒钟自动关闭
					APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
					NULL,											// 按键回调函数
					NULL,											// 用户回调函数私有参数
					XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
					//XM_VIEW_ALIGN_CENTRE,
					0												// ALERTVIEW视图的控制选项
					);
#endif
			break;


		case SYSTEM_EVENT_CARD_INSERT:			// SD卡插入事件(读写模式，文件系统检查OK)
			XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_CARD_INSERT................\r\n");
			XM_printf(">XMSYS_H264CodecGetForcedNonRecordingMode():%d\r\n", XMSYS_H264CodecGetForcedNonRecordingMode());

			#if 1
			// 标记录像状态
			//XMSYS_UvcSocketReportSDCardSystemEvent(SYSTEM_EVENT_CARD_INSERT);
			if(XMSYS_H264CodecGetForcedNonRecordingMode()==0)
			{
				XMSYS_H264CodecSetForcedNonRecordingMode(1);//设置成强制非录像模式
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
			// 标记卡重新检查
			if(AP_GetMenuItem (APPMENUITEM_VOICEASSISTANT_CARD_STATUS))
				XM_voice_prompts_insert_voice (XM_VOICE_ID_CARD_INSERT);
			*/

			#if 0
			XM_OpenAlertView (
					AP_ID_CARD_MESSAGE_CARD_INSERT,	// 信息文本资源ID
					AP_ID_CARD_ICON_SDCARD,			// 图片信息资源ID
					0,
					0,												// 按钮文字资源ID
					0,												// 按钮按下文字资源ID
					APP_ALERT_BKGCOLOR,
					5.0,											// 5秒钟自动关闭
					APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
					NULL,											// 按键回调函数
					NULL,											// 用户回调函数私有参数
					XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
					//XM_VIEW_ALIGN_CENTRE,
					0												// ALERTVIEW视图的控制选项
					);
			#endif
			//DesktopOnSystemCheck();
			ticket_to_card_verify_warning = 0;
			break;

		case SYSTEM_EVENT_CARD_INVALID:			// SD卡无法识别
			XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_CARD_INVALID................\r\n");
			//	SD卡无法识别 (检测到卡插入，但卡没有应答卡控的任何指令)
			XMSYS_UvcSocketReportSDCardSystemEvent (SYSTEM_EVENT_CARD_INVALID);
			//if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
			//	XM_voice_prompts_insert_voice (XM_VOICE_ID_ALARM);
#if CZS_USB_01
#else
			XM_OpenAlertView (
					AP_ID_CARD_MESSAGE_CARD_INVALID,	// 信息文本资源ID
					AP_ID_CARD_ICON_SDCARD,			// 图片信息资源ID
					0,
					0,												// 按钮文字资源ID
					0,												// 按钮按下文字资源ID
					APP_ALERT_BKGCOLOR,
					10.0,											// 10秒钟自动关闭
					APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
					NULL,											// 按键回调函数
					NULL,											// 用户回调函数私有参数
					XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
					//XM_VIEW_ALIGN_CENTRE,
					0												// ALERTVIEW视图的控制选项
					);
#endif
			break;

		case SYSTEM_EVENT_CARD_DISK_FULL:		// SD卡磁盘已满
			XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_CARD_DISK_FULL................\r\n");
			XMSYS_UvcSocketReportSDCardSystemEvent (SYSTEM_EVENT_CARD_DISK_FULL);
			if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS))
				XM_voice_prompts_insert_voice (XM_VOICE_ID_ALARM);
#if CZS_USB_01
#else
			XM_OpenAlertView (
					AP_ID_CARD_MESSAGE_CARD_DISK_FULL,	// 信息文本资源ID
					AP_ID_CARD_ICON_SDCARD,			// 图片信息资源ID
					0,
					0,												// 按钮文字资源ID
					0,												// 按钮按下文字资源ID
					APP_ALERT_BKGCOLOR,
					20.0,											// 10秒钟自动关闭
					APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
					NULL,											// 按键回调函数
					NULL,											// 用户回调函数私有参数
					XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
					//XM_VIEW_ALIGN_CENTRE,
					0												// ALERTVIEW视图的控制选项
					);
#endif
			break;

		// ------------------------------------
		//
		// *** 文件系统及视频项系统事件处理 ***
		//
		// ------------------------------------
		case SYSTEM_EVENT_VIDEOITEM_LOW_PREFORMANCE:		// 文件系统的参数需要重新配置，确保满足视频记录的要求
															// 一般指SD卡的簇大小太小，导致卡读写速度无法满足高清视频实时的写入
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
					AP_ID_CARD_INFO_LOWPREFORMANCE,	// 信息文本资源ID
					AP_NULLID,	//AP_ID_CARD_ICON_SDCARD,			// 图片信息资源ID
					2,												// 按钮个数
					dwButtonNormalTextID,					// 按钮文字资源ID
					dwButtonPressedTextID,					// 按钮按下文字资源ID
					APP_ALERT_BKGCOLOR,
					15.0,											// 15秒钟自动关闭
					APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
					fs_low_performance_alert_cb,			// 按键回调函数
					NULL,											// 用户回调函数私有参数
					XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
					//XM_VIEW_ALIGN_CENTRE,
					XM_ALERTVIEW_OPTION_ENABLE_CALLBACK			// ALERTVIEW视图的控制选项
					);
#endif
			break;
		}

		case SYSTEM_EVENT_VIDEOITEM_RECYCLE_CONSUMED:		// "锁定的录像文件已满，无法循环录像，请删除文件或重新格式化"
			XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_VIDEOITEM_RECYCLE_CONSUMED................\r\n");
			XMSYS_UvcSocketReportSDCardSystemEvent (SYSTEM_EVENT_VIDEOITEM_RECYCLE_CONSUMED);
#if CZS_USB_01
#else
			XM_OpenAlertView (
					AP_ID_CARD_INFO_RECYCLE_CONSUMED,	// 信息文本资源ID
					AP_ID_CARD_ICON_SDCARD,			// 图片信息资源ID
					0,
					0,												// 按钮文字资源ID
					0,												// 按钮按下文字资源ID
					APP_ALERT_BKGCOLOR,
					5.0,											// 5秒钟自动关闭
					APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
					NULL,											// 按键回调函数
					NULL,											// 用户回调函数私有参数
					XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
					//XM_VIEW_ALIGN_CENTRE,
					0												// ALERTVIEW视图的控制选项
					);
#endif
			break;

		case SYSTEM_EVENT_VIDEOITEM_LOW_SPEED:			// "SD卡写入速度较慢，可能需要重新格式化"
			XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_VIDEOITEM_LOW_SPEED................\r\n");

			XMSYS_UvcSocketReportSDCardSystemEvent (SYSTEM_EVENT_VIDEOITEM_LOW_PREFORMANCE);
#if CZS_USB_01
#else
			XM_OpenAlertView (
					AP_ID_CARD_INFO_LOWSPEED,	// 信息文本资源ID
					AP_ID_CARD_ICON_SDCARD,			// 图片信息资源ID
					0,
					0,												// 按钮文字资源ID
					0,												// 按钮按下文字资源ID
					APP_ALERT_BKGCOLOR,
					5.0,											// 5秒钟自动关闭
					APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
					NULL,											// 按键回调函数
					NULL,											// 用户回调函数私有参数
					XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
					//XM_VIEW_ALIGN_CENTRE,
					0												// ALERTVIEW视图的控制选项
					);
#endif
			break;

		case SYSTEM_EVENT_VIDEOITEM_LOW_SPACE:			// 循环录像的空间不足, 请删除其他文件或格式化
			XMSYS_UvcSocketReportSDCardSystemEvent (SYSTEM_EVENT_CARD_DISK_FULL);
			
			XM_OpenAlertView (
					AP_ID_CARD_INFO_LOWSPACE,	// 信息文本资源ID
					AP_ID_CARD_ICON_SDCARD,			// 图片信息资源ID
					0,
					0,												// 按钮文字资源ID
					0,												// 按钮按下文字资源ID
					APP_ALERT_BKGCOLOR,
					5.0,											// 5秒钟自动关闭
					APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
					NULL,											// 按键回调函数
					NULL,											// 用户回调函数私有参数
					XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
					//XM_VIEW_ALIGN_CENTRE,
					0												// ALERTVIEW视图的控制选项
					);
			
#if CZS_USB_01
#else
			XM_OpenRecycleVideoAlertView (
				AP_ID_CARD_INFO_LOWSPACE,		// 循环录像时间低于设定值
				APP_ALERT_BKGCOLOR,						// 背景色
				10.0,											// 10秒钟自动关闭
				APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
				XM_VIEW_ALIGN_BOTTOM,
				0
				);
#endif
			break;
#if 0
		case SYSTEM_EVENT_CCD0_LOST_CONNECT:		// 摄像头故障，无法识别
			XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_CCD0_LOST_CONNECT................\r\n");
			XM_voice_prompts_insert_voice (XM_VOICE_ID_ALARM );
#if CZS_USB_01
#else
			XM_OpenAlertView (
					AP_ID_CAMERA_INFO_CAMERADISCONNECT,	// 信息文本资源ID
					AP_ID_CAMERA_ICON_CAMERADISCONNECT,	// 图片信息资源ID
					0,
					0,												// 按钮文字资源ID
					0,												// 按钮按下文字资源ID
					APP_ALERT_BKGCOLOR,
					60.0,											// 60秒钟自动关闭
					APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
					NULL,											// 按键回调函数
					NULL,											// 用户回调函数私有参数
					XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
					0												// ALERTVIEW视图的控制选项
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
				// 显示屏连接中
				// TOGGLE切换模式
				if(AP_GetMenuItem (APPMENUITEM_MIC) == 0)
				{
					// MIC已关闭
					// 开启MIC录音
					AP_SetMenuItem (APPMENUITEM_MIC, 1);
				}
				else
				{
					// MIC已开启
					// 关闭MIC
					AP_SetMenuItem (APPMENUITEM_MIC, 0);
				}
			}
			else
			{
				// 显示屏未连接
				XM_OpenMicSoundVolumeSettingView ();
			}
			break;

		// 主电池电压变化事件
		case SYSTEM_EVENT_MAIN_BATTERY:
		XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_MAIN_BATTERY................\r\n");
		{
			// 读取主电池的当前状态
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
					AP_ID_MAINBATTERY_INFO_WORST,			// 信息文本资源ID
					AP_ID_BATTERY_ICON,						// 图片信息资源ID
					0,
					0,												// 按钮文字资源ID
					0,												// 按钮按下文字资源ID
					APP_ALERT_BKGCOLOR,
					10.0,											// 10秒钟自动关闭
					APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
					NULL,											// 按键回调函数
					NULL,											// 用户回调函数私有参数
					XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
					//XM_VIEW_ALIGN_CENTRE,
					0												// ALERTVIEW视图的控制选项
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
            //如果acc断开并且移动侦测选项为开，则发送邮件，打开移动侦测功能
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
			
		case SYSTEM_EVENT_USB_DISCONNECT:		// USB连接断开
			XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_USB_DISCONNECT................\r\n");
#if CZS_USB_01
#else
			XM_OpenAlertView (
					AP_ID_USB_DISCONNECT_TITLE,			// 信息文本资源ID
					AP_ID_USB_DISCONNECT_IMAGE,			// 图片信息资源ID
					0,
					0,												// 按钮文字资源ID
					0,												// 按钮按下文字资源ID
					APP_ALERT_BKGCOLOR,
					5.0,											// 5秒钟自动关闭
					APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
					NULL,											// 按键回调函数
					NULL,											// 用户回调函数私有参数
					XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
					//XM_VIEW_ALIGN_CENTRE,
					0												// ALERTVIEW视图的控制选项
					);
#endif
			break;

		case SYSTEM_EVENT_ONE_KEY_PROTECT:			// 快捷按键事件，紧急录像(一键锁定)
			XM_printf(">>>>>>>>>>>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_ONE_KEY_PROTECT................\r\n");
			{
				int urgent_record_time;
				DWORD dwButtonNormalTextID[1];
				DWORD dwButtonPressedTextID[1];
				// 获取紧急录像时间设置
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
											(float)urgent_record_time,		// 自动关闭时间
											APP_ALERT_BKGALPHA,
											//XM_VIEW_ALIGN_BOTTOM
											NULL,											// 按键回调函数
											NULL,											// 用户回调函数私有参数
											XM_VIEW_ALIGN_CENTRE,
											XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN|XM_ALERTVIEW_OPTION_ADJUST_COUNTDOWN
											);
#endif
				break;
			}

		case SYSTEM_EVENT_ONE_KEY_PHOTOGRAPH:// 快捷按键事件，一键拍照
			XM_printf(">>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_ONE_KEY_PHOTOGRAPH............\r\n");
			{
				// 播放提示语音
				unsigned int card_state = XM_GetFmlDeviceCap(DEVCAP_SDCARDSTATE);
					if(card_state == DEVCAP_SDCARDSTATE_UNPLUG)
					{
						// 卡已拔出
						XM_OpenAlertView (
							AP_ID_CARD_MESSAGE_CARD_WITHDRAW,	// 信息文本资源ID
							AP_ID_CARD_ICON_SDCARD,			// 图片信息资源ID
							0,
							0,												// 按钮文字资源ID
							0,												// 按钮按下文字资源ID
							APP_ALERT_BKGCOLOR,
							10.0,											// 10秒钟自动关闭
							APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
							NULL,											// 按键回调函数
							NULL,											// 用户回调函数私有参数
							XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
							//XM_VIEW_ALIGN_CENTRE,
							0												// ALERTVIEW视图的控制选项
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

						// 卡文件系统错误
						XM_OpenAlertView (
							AP_ID_CARD_MESSAGE_CARD_FSERROR,		// 信息文本资源ID
							AP_ID_CARD_ICON_SDCARD,												// 图片信息资源ID
							2,												// 按钮个数
							dwButtonNormalTextID,					// 按钮文字资源ID
							dwButtonPressedTextID,					// 按钮按下文字资源ID
							APP_ALERT_BKGCOLOR,
							10.0,											// 10秒钟自动关闭
							APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
							fs_access_error_alert_cb,							// 按键回调函数
							NULL,											// 用户回调函数私有参数
							XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
							//XM_VIEW_ALIGN_CENTRE,
							XM_ALERTVIEW_OPTION_ENABLE_CALLBACK	// ALERTVIEW视图的控制选项
							);
					}
					else if(card_state == DEVCAP_SDCARDSTATE_INVALID)
					{
						// 卡无效(卡无法识别)
						XM_OpenAlertView (
							AP_ID_CARD_MESSAGE_CARD_INVALID,	// 信息文本资源ID
							AP_ID_CARD_ICON_SDCARD,			// 图片信息资源ID
							0,
							0,												// 按钮文字资源ID
							0,												// 按钮按下文字资源ID
							APP_ALERT_BKGCOLOR,
							10.0,											// 10秒钟自动关闭
							APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
							NULL,											// 按键回调函数
							NULL,											// 用户回调函数私有参数
							XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
							//XM_VIEW_ALIGN_CENTRE,
							0												// ALERTVIEW视图的控制选项
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
						//把拍照期间的命令清掉，解决多次按拍照慢的问题
						//PhtographStart=0;
						XM_FlushMessage ();
					}
				break;
			}

		#if 0
		case SYSTEM_EVENT_GPSBD_DISCONNECT:					// GPSBD已断开
			XM_OpenAlertView (
					AP_ID_GPSBD_DISCONNECT,					// 信息文本资源ID
					AP_ID_GPSBD_ICON,							// 图片信息资源ID
					0,												// 按钮个数
					0,												// 按钮文字资源ID
					0,												// 按钮按下文字资源ID
					APP_ALERT_BKGCOLOR,
					5.0,											// 5秒钟自动关闭
					APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
					NULL,											// 按键回调函数
					NULL,											// 用户回调函数私有参数
					XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
					//XM_VIEW_ALIGN_CENTRE,
					0												// ALERTVIEW视图的控制选项
					);
			break;
		case SYSTEM_EVENT_GPSBD_CONNECT_LOCATE_OK:		// GPSBD已定位
			XM_OpenAlertView (
					AP_ID_GPSBD_CONNECT_LOCATE_OK,	// 信息文本资源ID
					AP_ID_GPSBD_ICON,						// 图片信息资源ID
					0,												// 按钮个数
					0,												// 按钮文字资源ID
					0,												// 按钮按下文字资源ID
					APP_ALERT_BKGCOLOR,
					5.0,											// 5秒钟自动关闭
					APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
					NULL,											// 按键回调函数
					NULL,											// 用户回调函数私有参数
					XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
					//XM_VIEW_ALIGN_CENTRE,
					0												// ALERTVIEW视图的控制选项
					);
			break;
		#endif
		
		case SYSTEM_EVENT_SYSTEM_UPDATE_FILE_CHECKED:	// 找到合法的系统升级文件
			XM_printf(">>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_SYSTEM_UPDATE_FILE_CHECKED............\r\n");
			//OpenSystemUpdateDialog ();
			AP_OpenSystemUpdateView(0);
			break;

		case SYSTEM_EVENT_BL_OFF:			// 关背光
			XM_printf(">>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_BL_OFF............\r\n");

			#if 1
			{
				// OSD关闭
				XM_printf ("SYSTEM_EVENT_BL_OFF\n");
				//XM_SetFmlDeviceCap (DEVCAP_OSD, 0);
				// 关闭背光
				XM_SetFmlDeviceCap (DEVCAP_BACKLIGHT, 0);

				XM_printf ("BL off\n");
			}
			#endif
			break;

			
		case SYSTEM_EVENT_BL_ON:			// 开背光
			XM_printf(">>>>>>>>>>>>>XM_DefaultSystemEventProc, SYSTEM_EVENT_BL_ON............\r\n");
			{
				XM_printf ("BL on\n");
				// OSD开启
				//XM_SetFmlDeviceCap (DEVCAP_OSD, 1);
				// 背光开启
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
		// 执行"系统升级"命令
		AP_OpenSystemUpdateView (0);
		return;
	}
	// 将alert弹出
	XM_PullWindow (0);
}

// 打开系统升级提示对话框
void OpenSystemUpdateDialog (void)
{
	// 2个按键, YES/CANCEL
	DWORD dwButtonNormalTextID[2];
	DWORD dwButtonPressedTextID[2];


	if(XMSYS_system_update_get_version() == 0xFFFFFFFF)
	{
		// 自动安装的升级版本, "无确认升级模式"
		// 执行"系统升级"命令
		AP_OpenSystemUpdateView (0);
		return;
	}

	dwButtonNormalTextID[0] = AP_ID_SYSTEMUPDATE_YES;
	dwButtonNormalTextID[1] = AP_ID_SYSTEMUPDATE_CANCEL;
	dwButtonPressedTextID[0] = AP_ID_SYSTEMUPDATE_YES;
	dwButtonPressedTextID[1] = AP_ID_SYSTEMUPDATE_CANCEL;
	XM_OpenAlertView (
					AP_ID_SYSTEMUPDATE_TITLE,				// 信息文本资源ID
					0,												// 图片信息资源ID
					2,												// 按钮个数
					dwButtonNormalTextID,					// 按钮文字资源ID
					dwButtonPressedTextID,					// 按钮按下文字资源ID
					APP_ALERT_BKGCOLOR,						// Alert视窗背景色
					30.0,											// 设置30秒钟自动关闭, 0 表示禁止自动关闭
					APP_ALERT_BKGALPHA,						// 信息视图的视窗Alpha因子，0.0 ~ 1.0
					SystemUpdateDialogCallBack,			// 按键回调函数
					NULL,											// 用户回调函数私有参数
					XM_VIEW_ALIGN_BOTTOM,					// 视图的显示对齐设置信息
					//XM_VIEW_ALIGN_CENTRE,
					XM_ALERTVIEW_OPTION_ENABLE_CALLBACK	// 使能按键回调函数功能, ALERTVIEW视图的控制选项
					);
}

// #define	XM_CARD			0x10	// SD卡插拔事件
										// lp保留为0
										// wp = 0, SD卡拔出事件
										// wp = 1, SD卡插入(写保护)
										// wp = 2, SD卡插入(读写允许)

VOID DesktopOnSystemEvent (XMMSG *msg)
{
	XM_printf(">>>>>>>>DesktopOnSystemEvent, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);

	XM_DefaultSystemEventProc(XMHWND_HANDLE(Desktop), msg);
}


// 消息处理函数定义
XM_MESSAGE_MAP_BEGIN (Desktop)
	XM_ON_MESSAGE (XM_KEYDOWN, DesktopOnKeyDown)
	XM_ON_MESSAGE (XMKEY_REPEAT, DesktopOnKeyRepeatDown)
	XM_ON_MESSAGE (XMKEY_LONGTIME, DesktopOnKeyLongTimeDown)
	XM_ON_MESSAGE (XM_ENTER, DesktopOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, DesktopOnLeave)
	XM_ON_MESSAGE (XM_TIMER, DesktopOnTimer)
	XM_ON_MESSAGE (XM_SYSTEMEVENT, DesktopOnSystemEvent)
XM_MESSAGE_MAP_END

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, Desktop, 1, 0, 0, 0, HWND_VIEW)


