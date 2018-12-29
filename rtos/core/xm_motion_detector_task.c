
//移动侦测
#include "xm_h264_codec.h"
#include "RTOS.h"		// OS头文件
#include "xm_key.h"
#include "xm_core.h"
#include "Xm_motion_detector.h"
#include "xm_user.h"
#include "xm_app_menudata.h"

#define MOTION_DET_ENABLE TRUE

unsigned char motion_start_detect_opt_flag = FALSE;
unsigned char motion_stop_detect_opt_flag = FALSE;

OS_STACKPTR int StackMotion[XMSYS_MAIN_TASK_STACK_SIZE/4];          /* Task stacks */
OS_TASK MotionTask;                        /* Task-control-blocks */

//extern XMBOOL EnterSystemStatus;
#define MOTION_EVENT_START 			0X01
#define MOTION_EVENT_TIMER			0X02
#define MOTION_EVENT_STOP			0X04
#define MOTION_EVENT_SENSITIVITY	0X08

#define MAX_FRAME_CNT				4

static OS_TIMER MotionTimer;
static XMBOOL MotionTimerStatus = FALSE;
static unsigned int MotionTimeCount = 0;
static unsigned char move_frame_cnt =  0;//运动帧数
static unsigned char stop_frame_cnt =  0;//禁止帧数

void MotionDetectMoveProcess(void);
void MotionDetectNotMoveProcess(void);

XMBOOL IsMotionDetectStarted()
{
    return MotionTimerStatus;
}

void MotionDetectStart(void)
{
	OS_SignalEvent(MOTION_EVENT_START, &MotionTask);
}

void MotionDetectStop(void)
{
	OS_SignalEvent(MOTION_EVENT_STOP, &MotionTask);
}

void MotionDetectSetSensitivity(void)
{
	OS_SignalEvent(MOTION_EVENT_SENSITIVITY, &MotionTask);
}

void MotionTimerCallback(void)
{
    if(MotionTimerStatus)
    {
    	MotionTimeCount++;
    	OS_SignalEvent(MOTION_EVENT_TIMER, &MotionTask); /* 通知事件 */
    	OS_RetriggerTimer(&MotionTimer);
    }
}

void MotionSensitivitySet(void)
{
	//printf("motion set sensitivity  = %d \n\r",CurMotionSensitivity);
	
	xm_motion_detector_config(320,240,320);
	xm_motion_detector_set_pixel_threshold(1000);//判定每个像素点是否是移动点
	
	//if(CurMotionSensitivity == 1)
	//	xm_motion_detector_set_motion_threshold(150);
	//else if(CurMotionSensitivity == 2)
		xm_motion_detector_set_motion_threshold(3000);
	//else
	//	xm_motion_detector_set_motion_threshold(50);
}

void MotionDetectMoveProcess(void)
{//录像状态停止录像
	if(XM_GetFmlDeviceCap(DEVCAP_VIDEO_REC) == DEVCAP_VIDEO_REC_STOP)
	{
		XM_KeyEventProc(VK_AP_SWITCH, XM_KEYDOWN);
	}
}


void MotionDetectNotMoveProcess(void)
{
	if(XM_GetFmlDeviceCap(DEVCAP_VIDEO_REC) == DEVCAP_VIDEO_REC_START)
	{
		XM_KeyEventProc(VK_AP_SWITCH, XM_KEYDOWN);
	}
}


static void MotionTaskFun(void)
{
	unsigned char MotionEvent;

	#if MOTION_DET_ENABLE
	xm_motion_detector_init();
	MotionSensitivitySet();
	#endif
	
	#if MOTION_DET_ENABLE
	unsigned int length = 320*240;
	unsigned int  pixel_threshold;
	unsigned int  motion_count;
	unsigned int  image_threshold;
	unsigned int  motion_detect_result;
	
	MotionTimeCount = 0;
	MotionTimerStatus = FALSE;
	//OS_CREATETIMER(&MotionTimer, MotionTimerCallback, 1000);
	#endif

	XM_printf(">>>>>>MotionTaskFun......\r\n");
	//周期性设置并清除watchdog, 保护异常重新启动
	while(1)
	{
		MotionEvent = OS_WaitEvent(MOTION_EVENT_START|MOTION_EVENT_STOP|MOTION_EVENT_TIMER|MOTION_EVENT_SENSITIVITY);

		if((MotionEvent &MOTION_EVENT_START) && (!MotionTimerStatus))
		{
			printf("----------->> motion start \n\r");
			MotionTimeCount = 0;
			MotionTimerStatus = TRUE;
			OS_RetriggerTimer(&MotionTimer);
		}
		else if((MotionEvent & MOTION_EVENT_STOP)&&MotionTimerStatus)
		{
			printf("----------->> motion stop \n\r");
			MotionTimerStatus = FALSE;
			//OS_DeleteTimer(&MotionTimer);
		}
		else if((MotionEvent & MOTION_EVENT_TIMER)&& get_ch2_video_image())
		{
			//XM_printf(">>>>MotionTimeCount:%d\r\n", MotionTimeCount);

			if( xm_motion_detector_monitor_image((char *)get_ch2_video_image()) )
			{
				XM_printf(">>>>move_frame_cnt:%d\r\n", move_frame_cnt);
				stop_frame_cnt = 0;
				motion_stop_detect_opt_flag = FALSE;
				move_frame_cnt++;
				if(move_frame_cnt>=MAX_FRAME_CNT)//大于50帧为有效
				{
					move_frame_cnt = 0;
					//MotionDetectStop();
					if(!motion_start_detect_opt_flag)
					{
						HW_LCD_BackLightOn();
						AP_SetMenuItem(APPMENUITEM_POWER_STATE, POWER_ON);
						motion_start_detect_opt_flag = TRUE;
						MotionDetectMoveProcess();//发送按键消息,开始录像
					}
				}
			}
			else
			{
				XM_printf(">>>>stop_frame_cnt:%d\r\n", stop_frame_cnt);
				move_frame_cnt = 0;
				motion_start_detect_opt_flag = FALSE;
				stop_frame_cnt++;
				if(stop_frame_cnt>=20)//大于50帧为有效
				{
					stop_frame_cnt = 0;
					//MotionDetectStop();
					if(!motion_stop_detect_opt_flag)
					{
						HW_LCD_BackLightOff();
						AP_SetMenuItem(APPMENUITEM_POWER_STATE, POWER_OFF);
						motion_stop_detect_opt_flag = TRUE;
						MotionDetectNotMoveProcess();//发送按键消息,开始录像
					}
				}
			}
			
			#if 0
			if( xm_motion_detector_monitor_image((char *)get_front_video_image()) && (MotionTimeCount > 10))
			{
				XM_printf(">>>>motion start...\n\r");
				
				MotionDetectStop();
				if(!motion_detect_opt_flag)
				{
					motion_detect_opt_flag = TRUE;
					MotionDetectMoveProcess();//发送按键消息,开始录像
				}
			}
			else if(MotionTimeCount > 600)
			{
				XM_printf(">>>>motion end...\n\r");
				
				MotionDetectStop();
				if(motion_detect_opt_flag)
				{
					MotionTimeCount = 0;
					motion_detect_opt_flag = FALSE;
					MotionDetectNotMoveProcess();//发送按键消息,停止录像
				}
			}
			else
			{
				printf(".");
			}
			#endif
		}
		else if(MotionEvent & MOTION_EVENT_SENSITIVITY)
		{//设置灵敏度
			MotionSensitivitySet();
		}
	}
}






void MotionTaskInit(void)
{
    OS_CREATETIMER(&MotionTimer, MotionTimerCallback, 1000);
	OS_CREATETASK(&MotionTask, "MoitonTask", MotionTaskFun, 200, StackMotion);
}


