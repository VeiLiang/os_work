//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_win_user.c
//	  内核消息处理函数
//
//	Revision history
//
//		2010.09.01	ZhuoYongHong Initial version
//		2012.01.20	ZhuoYongHong 增加Alpha通道处理
//
//****************************************************************************
#include <string.h>
#include <xm_user.h>
#include <xm_base.h>
#include <xm_dev.h>
#include <xm_widget.h>
#include <xm_assert.h>
#include <xm_queue.h>
#include <xm_key.h>
#include <xm_printf.h>
#include "xm_device.h"
#include "rom.h"
#include <xm_tester.h>
#include <hw_osd_layer.h>
#include <xm_osd_layer.h>
#include <xm_osd_layer_animate.h>
#include "types.h"
#include "xm_app_menudata.h"
// 消息队列定义
typedef struct _kmsg
{
	struct _kmsg*	prev;
	struct _kmsg*	next;
	XMMSG				msg;
} kmsg_s, *kmsg_t;

// 根据桌面与基准屏之间的关系计算视窗对齐方式及缩放因子
static void XM_init_view_reference_coordinate (void);

extern  int ShowLogo (UINT32 LogoAddr,UINT32 LogoSize);
extern int PowerOnShowLogo;

static kmsg_s	kmsg_buff[XM_MAX_MSG];
static queue_s	kmsg_pool;
static queue_s	mcb_post_msg;


// 视窗栈
static HWND_NODE	window_stack[MAX_HWND_STACK];

// 视窗中Widget标志
static BYTE			widget_flag[MAX_STACK_WIDGET_COUNT];

// 控件(Widget)的用户数据
static VOID *		widget_userdata[MAX_STACK_WIDGET_COUNT];

// 视窗栈中视窗的个数
static BYTE			window_count;

// 视窗栈中Widget的个数
static BYTE			widget_count;

//	XM_JUMP_POPDEFAULT = 0,			//	将视窗栈中所有视窗清除(除去桌面)，然后跳到新的视窗
//	XM_JUMP_POPDESKTOP,				// 将视窗栈中所有视窗清除(包括桌面)，然后跳到新的视窗
//	XM_JUMP_POPTOPVIEW,				// 将视窗栈的栈顶视窗清除，然后跳到新的视窗
static XM_JUMP_TYPE		jump_type;		// 窗口跳转时是否弹出顶层视窗/桌面。

static HANDLE	jumpWnd;
static HANDLE	pushWnd;
static HANDLE	pullWnd;

static DWORD	customData;		// 窗口创建时定制数据，与特定窗口相关。
										// 通过定制数据，可对窗口行为进行定制
										// 定制属性通过XM_ENTER消息的lp参数传入
										// 0 表示缺省属性

// 视窗创建时animating效果的缺省方向
static UINT		view_animating_direction = XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP;

// view比例投射控制
// 根据桌面视窗的定义坐标( _x, _y, _cx, _cy ) 与 基准屏的基准坐标 (0, 0, LCD_XDOTS, LCD_YDOTS) 之间的关系，
//		计算每个视窗映射到目标投射系统之间的比例、对齐方式等
#define	XM_VIEW_ALIGN_HORZ_SCALE		0x00000001			// 视窗水平比例对齐
#define	XM_VIEW_ALIGN_HORZ_LEFT		0x00000002			// 视窗水平向左对齐(相对于基准屏)
#define	XM_VIEW_ALIGN_HORZ_RIGHT		0x00000004			// 视窗水平向右对齐(相对于基准屏)
#define	XM_VIEW_ALIGN_HORZ_CENTRE	0x00000008			// 视窗水平居中对齐(相对于基准屏)
#define	XM_VIEW_ALIGN_VERT_SCALE		0x00000010			// 视窗垂直比例对齐
#define	XM_VIEW_ALIGN_VERT_TOP		0x00000020			// 视窗垂直向上对齐(相对于基准屏)
#define	XM_VIEW_ALIGN_VERT_BOTTOM	0x00000040			// 视窗垂直向下对齐(相对于基准屏)
#define	XM_VIEW_ALIGN_VERT_CENTRE	0x00000080			// 视窗垂直居中对齐(相对于基准屏)

static float	view_default_horz_ratio;		// 系统缺省视窗水平缩放比例因子(根据桌面与基准屏之间的关系计算)
static float	view_default_vert_ratio;		//	系统缺省视窗垂直缩放比例因子(根据桌面与基准屏之间的关系计算)
static unsigned int	view_default_align;		// 系统缺省对齐方式(根据桌面与基准屏之间的关系计算)

XMHWND_DECLARE(Desktop)
XMHWND_DECLARE(FactoryView)

extern void	winmem_save_environment (unsigned char *buff, unsigned int size);
extern void	winmem_restore_environment (unsigned char *buff, unsigned int size);

// 保存XM系统的运行环境(内部函数)
void xm_save_environment (unsigned char *buff, unsigned int size)
{
	unsigned char *start = buff;
	memcpy (buff, window_stack, sizeof(window_stack));
	buff += sizeof(window_stack);
	memcpy (buff, widget_flag, sizeof(widget_flag));
	buff += sizeof(widget_flag);
	memcpy (buff, widget_userdata, sizeof(widget_userdata));
	buff += sizeof(widget_userdata);
	memcpy (buff, &window_count, sizeof(window_count));
	buff += sizeof(window_count);
	memcpy (buff, &widget_count, sizeof(widget_count));
	buff += sizeof(widget_count);
	memcpy (buff, &view_animating_direction, sizeof(view_animating_direction));
	buff += sizeof(view_animating_direction);

	winmem_save_environment (buff, buff - start);
}

// 恢复XM系统的运行环境(内部函数)
void xm_restore_environment (unsigned char *buff, unsigned int size)
{
	int i;
	unsigned char *start = buff;
	memcpy (window_stack, buff, sizeof(window_stack));
	buff += sizeof(window_stack);
	memcpy (widget_flag, buff, sizeof(widget_flag));
	buff += sizeof(widget_flag);
	memcpy (widget_userdata, buff, sizeof(widget_userdata));
	buff += sizeof(widget_userdata);
	memcpy (&window_count, buff, sizeof(window_count));
	buff += sizeof(window_count);
	memcpy (&widget_count, buff, sizeof(widget_count));
	buff += sizeof(widget_count);
	memcpy (&view_animating_direction, buff, sizeof(view_animating_direction));
	buff += sizeof(view_animating_direction);

	winmem_restore_environment (buff, buff - start);

	// 20140511 ZhuoYongHong
	// 调整已压栈的视窗的可视位置信息
	for (i = 0; i < window_count; i ++)
	{
		HWND_NODE *node;
		node = &window_stack[i];
		if(node->hwnd)
		{
			XMHWND *pWnd;
			// 获取视窗结构的指针
			pWnd = ADDRESS_OF_HANDLE (node->hwnd);

			// 根据桌面视窗的定义坐标( _x, _y, _cx, _cy ) 与 基准屏的基准坐标 (0, 0, LCD_XDOTS, LCD_YDOTS) 之间的关系，
			// 计算每个视窗映射到目标投射系统之间的比例、对齐方式等

			pWnd->view_x = (XMCOORD)(pWnd->_x * view_default_horz_ratio + 0.5);
			pWnd->view_cx = (XMCOORD)(pWnd->_cx * view_default_horz_ratio + 0.5);
			pWnd->view_y = (XMCOORD)(pWnd->_y * view_default_vert_ratio + 0.5);;
			pWnd->view_cy = (XMCOORD)(pWnd->_cy * view_default_vert_ratio + 0.5);
		}
	}
}

static kmsg_t new_kmsg	(WORD msg, WORD wp, DWORD lp)
{
	kmsg_t kmsg;

	if(queue_empty (&kmsg_pool))
	{
		//XM_printf ("fatal error, failed to new_kmsg because msg pool exhausted\n");
		return (kmsg_t)NULL;
	}

	kmsg = (kmsg_t)queue_delete_next (&kmsg_pool);

	kmsg->msg.message	= msg;
	kmsg->msg.wp		= wp;
	kmsg->msg.lp		= lp;
	if(msg == XM_SYSTEMEVENT)
		kmsg->msg.option = XMMSG_OPTION_SYSTEMEVENT_DEFAULT_PROCESS;
	else
		kmsg->msg.option	= 0;

	kmsg->prev			= (kmsg_t)0;
	kmsg->next			= (kmsg_t)0;
	return kmsg;
}

static void del_kmsg (kmsg_t kmsg)
{
	if(kmsg != NULL)
	{
		kmsg->msg.message = 0;
		kmsg->msg.wp = 0;
		kmsg->msg.lp = 0;
		kmsg->msg.option = 0;
		queue_insert ((queue_s *)kmsg, &kmsg_pool);
	}
}


static HWND_NODE *get_node (void)
{
	if(window_count < 1)
		return NULL;

	return &window_stack[window_count - 1];
}

static HWND_NODE * get_node_from_hwnd (HANDLE hWnd)
{
	HWND_NODE *node;
	BYTE	i;

	if(hWnd == NULL)
		return NULL;

	node = NULL;
	for (i = 0; i < window_count; i++)
	{
		if(window_stack[i].hwnd == hWnd)
		{
			node = &window_stack[i];
			break;
		}
	}
	return node;
}


// 消息系统、视窗系统初始化
void winuser_init (void)
{
	BYTE i;

	// 消息队列初始化
	queue_initialize (&mcb_post_msg);

	memset (&kmsg_buff, 0, sizeof(kmsg_buff));
	queue_initialize (&kmsg_pool);
	for (i = 0; i < XM_MAX_MSG; i++)
	{
		queue_insert ((queue_s *)&kmsg_buff[i], &kmsg_pool);
	}

	memset (window_stack, 0, sizeof(window_stack));
	memset (widget_flag, 0, sizeof(widget_flag));
	memset (widget_userdata, 0, sizeof(widget_userdata));
	window_count = 0;
	widget_count = 0;

	jump_type = XM_JUMP_POPDEFAULT;

	jumpWnd = NULL;
	pushWnd = NULL;
	pullWnd = NULL;

	customData = 0;

	view_animating_direction = XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP;

	XM_osd_framebuffer_init ();

	// 根据桌面与基准屏之间的关系计算视窗对齐方式及缩放因子
	XM_init_view_reference_coordinate ();

}

// 消息系统、视窗系统关闭
void winuser_exit (void)
{
	XM_osd_framebuffer_exit ();
	//winuser_init ();
}

// 向消息队列投递消息
XMBOOL	XM_PostMessage (WORD message, WORD wp, DWORD lp)
{
	kmsg_t	kmsg;
	XMMSG	*msg;

	// 检查是否正在等待视窗PUSH、JUMP或PULL操作
	if(jumpWnd || pushWnd || pullWnd)
	{
		// 检查是否是系统事件消息, 允许在任何时候投递
		if(message == XM_SYSTEMEVENT)
		{
		}
		// 允许用户自定义消息派发。允许不同窗口之间传递应用自定义消息
		else if(message < XM_USER)
			return 0;
	}
	#if 0
	if(message == XM_KEYDOWN)
	{
		// 按键音
		XM_Beep (XM_BEEP_KEYBOARD);

	}
	#endif

	// 检查是否是自动测试。自动测试下禁止TIMER消息
	//if(TesterSendCommand (TESTER_CMD_TEST, NULL))
	//{
	//	// Autotest running, disable TIMER to violate autotest process
	//	if(message == XM_TIMER)
	//	{
	//		return TRUE;
	//	}
	//}

	// 检查重复的XM_MCI消息
	if(message == XM_MCI)
	{
		// 检查消息队列是否为空
		if(!queue_empty (&mcb_post_msg))
		{
			// 遍历队列中的消息
			kmsg = (kmsg_t)queue_next (&mcb_post_msg);
			while(kmsg != (kmsg_t)&mcb_post_msg)
			{
				msg = &kmsg->msg;
				if(msg->message == XM_MCI)
				{
					return 1;
				}
				kmsg = (kmsg_t)queue_next ((queue_s *)kmsg);
			}
		}
	}
	else if(message == XM_TIMER)
	{
		// 检查消息队列是否为空
		if(!queue_empty (&mcb_post_msg))
		{
			// 遍历队列中的消息
			kmsg = (kmsg_t)queue_next (&mcb_post_msg);
			while(kmsg != (kmsg_t)&mcb_post_msg)
			{
				msg = &kmsg->msg;
				if(msg->message == XM_TIMER && wp == msg->wp)
				{
					// 找到同一个ID的TIMER消息
					// 删除该消息，将新消息加入到队列尾部
					queue_delete ((queue_s *)kmsg);
					// 将消息单元释放
					del_kmsg (kmsg);
					break;
				}
				kmsg = (kmsg_t)queue_next ((queue_s *)kmsg);
			}
		}
	}
	// 检查重复的PAINT消息
	else if(message == XM_PAINT)
	{
		// 检查消息队列是否为空
		if(!queue_empty (&mcb_post_msg))
		{
			// 遍历队列中的消息
			kmsg = (kmsg_t)queue_next (&mcb_post_msg);
			while(kmsg != (kmsg_t)&mcb_post_msg)
			{
				msg = &kmsg->msg;
				if(msg->message == XM_PAINT)
				{
					// 找到PAINT消息
					return 1;
				}
				kmsg = (kmsg_t)queue_next ((queue_s *)kmsg);
			}
		}
	}

	kmsg = new_kmsg (message, wp, lp);
	if(kmsg == NULL)
	{
		// 消息队列满。强制删除所有未处理的KeyDown，KeyUp消息
		// 遍历队列中的消息
		kmsg = (kmsg_t)queue_next (&mcb_post_msg);
		while(kmsg != (kmsg_t)&mcb_post_msg)
		{
			msg = &kmsg->msg;
			if(msg->message == XM_KEYDOWN || msg->message == XM_KEYUP)
			{
				// 找到同一个ID的TIMER消息
				// 删除该消息，将新消息加入到队列尾部
				queue_delete ((queue_s *)kmsg);
				// 将消息单元释放
				del_kmsg (kmsg);
				break;
			}
			kmsg = (kmsg_t)queue_next ((queue_s *)kmsg);
		}

		kmsg = new_kmsg (message, wp, lp);
		if(kmsg == NULL)
		{
			// 系统严重错误
			return 0;
		}
	}

	queue_insert ((queue_s *)kmsg, &mcb_post_msg);

	return 1;
}

XMBOOL	XM_PostQuitMessage (WORD dwExitCode)
{
	//XM_printf ("PostQuitMessage %d\n", dwExitCode);
	return XM_PostMessage (XM_QUIT, 0, dwExitCode);
}


// 在消息队列队首处插入指定的消息
XMBOOL	XM_InsertMessage (WORD message, WORD wp, DWORD lp)
{
	kmsg_t	kmsg;
	XMMSG		*msg;
	queue_s *head;

	// XM_printf ("XM_InsertMessage message=%d, wp=%d, lp=%d\n", message, wp, lp);

	// 检查是否正在等待视窗PUSH、JUMP或PULL操作
	if(jumpWnd || pushWnd || pullWnd)
		return 0;

	kmsg = new_kmsg (message, wp, lp);
	if(kmsg == NULL)
	{
		// 消息队列满。强制删除所有未处理的KeyDown，KeyUp消息
		// 遍历队列中的消息
		kmsg = (kmsg_t)queue_next (&mcb_post_msg);
		while(kmsg != (kmsg_t)&mcb_post_msg)
		{
			msg = &kmsg->msg;
			if(msg->message == XM_KEYDOWN || msg->message == XM_KEYUP)
			{
				// 找到同一个ID的TIMER消息
				// 删除该消息，将新消息加入到队列尾部
				queue_delete ((queue_s *)kmsg);
				// 将消息单元释放
				del_kmsg (kmsg);
				break;
			}
			kmsg = (kmsg_t)queue_next ((queue_s *)kmsg);
		}

		kmsg = new_kmsg (message, wp, lp);
		if(kmsg == NULL)
		{
			// 系统严重错误
			return 0;
		}
	}

	if(queue_empty (&mcb_post_msg))
	{
		queue_insert ((queue_s *)kmsg, &mcb_post_msg);
	}
	else
	{
		// 插入到队首消息的前面
		head = queue_head (&mcb_post_msg);
		queue_insert ((queue_s *)kmsg, head);
	}

	return 1;
}

void XM_PollEvent (XMBOOL bBlockCaller)
{
	// 进入中断禁止模式
	XM_lock ();


	XM_unlock ();
}

unsigned char First_LCD_Close = TRUE;
extern BOOL Close_Audio_Sound;
extern BOOL Close_Brighness;
VOID PowerOn_PowerKey(VOID)
{
    Close_Audio_Sound = TRUE;
	Close_Brighness = 1;
    HW_LCD_BackLightOn();
    //打开摄像头的电压
    //OPEN_Video_System();
    //开机声音 -声音由NVP6214 判定到格式之后自动打开
    //HW_Auido_SetLevel(AP_GetAudo_PWM()); //声音调成最低
    //APP_SetAudio_Sound(0); //
    //关机状态,记忆当前状态
    APP_SetPowerOn_Memory(Close_Brighness);//设置当前记忆的模式
}
static VOID PollHardwareEvent (XMBOOL bBlockCaller)
{
	WORD key, mod;
	DWORD point, type;
	BYTE HasEvent = 0;

	// 查询并等待硬件事件
	//XM_PollEvent (bBlockCaller);

	// 按键事件处理
	if(XM_KeyDriverPoll())
	{
		HasEvent = 1;
		while(XM_KeyDriverGetEvent (&key, &mod))
		{
			if(((key == VK_POWER)||(key == REMOTE_KEY_POWER))&& !get_parking_trig_status())
			{
               if(AP_GetMenuItem(APPMENUITEM_POWER_STATE)==POWER_ON) 
               {
                   HW_LCD_BackLightOff();
                  // XM_printf("ZY......%s %d \n",  __FUNCTION__,__LINE__);
                   AP_SetMenuItem(APPMENUITEM_POWER_STATE,POWER_OFF);

               }
               else if(AP_GetMenuItem(APPMENUITEM_POWER_STATE)==POWER_OFF) 
			   {
                  //XM_printf("ZY......%s %d \n",  __FUNCTION__,__LINE__);

                   AP_SetMenuItem(APPMENUITEM_POWER_STATE,POWER_ON);
                   HW_LCD_BackLightOn();

			   }
			   APP_SaveMenuData();
			   #if 0
				// 倒车不关机
			   if(Close_Audio_Sound) { //第一次
    			    //PowerOnShowLogo=0;//长按关机LOGO
    			    //关闭背光
    			    Close_Audio_Sound = FALSE;
                    HW_LCD_BackLightOff();
                    LED_Close_Brightness();//关灯
                    //关闭摄像头的电压
                    //Close_System();
                    //关声音
                    HW_Auido_SetLevel(0); //声音调成最低
                    XM_voice_prompts_remove_all_voice();
                    //APP_SetAudio_Sound(0); //
                    //关机状态,记忆当前状态
                    APP_SetPowerOn_Memory(Close_Brighness);//设置当前记忆的模式
                    APP_SaveMenuData ();
                    //清除所有的声音
    			    //if(AP_GetLogo())
                    //    ShowLogo (ROM_T18_COMMON_DESKTOP_LOGO_POWEROFF_JPG,ROM_T18_COMMON_DESKTOP_LOGO_POWEROFF_JPG_SIZE);
    	            //OS_Delay (1000);
    				//XM_PostQuitMessage (0);
               }else { //机器重启
                    //把自动关机模式关闭
                    XM_PostQuitMessage (XMEXIT_REBOOT); //按键控制,是为开机 power保持不变
               }
               #endif
			}
			// 20180129 zhuoyonghong
			// 增加软件重启功能
			else if(key == VK_REBOOT)
			{
				XM_PostQuitMessage (XMEXIT_REBOOT);
			}
			// 将系统事件(卡事件、文件系统异常、电池异常等)的模拟按键消息翻译为系统事件消息
			else if(key == VK_AP_SYSTEM_EVENT)
			{
			system_event_process:
				// 将系统事件按键消息翻译为系统事件消息

				// 判断是否是外部视频输出设备(AVOUT、HDMI)接入到系统的触发事件
				if(	mod == SYSTEM_EVENT_AVOUT_PLUGOUT
					||	mod == SYSTEM_EVENT_AVOUT_PLUGIN
					|| mod == SYSTEM_EVENT_HDMI_PLUGOUT
					||	mod == SYSTEM_EVENT_HDMI_PLUGIN
					)
				{
					// AVOUT/HDMI 拔插事件, 执行分辨率切换过程
					XM_PostQuitMessage (XMEXIT_CHANGE_RESOLUTION);
				}
				else
				{
					XM_PostMessage (XM_SYSTEMEVENT, mod, 0);
				}
			}
			else if(key == VK_AP_VIDEOSTOP)
			{
				// 将视频停止按键消息翻译为视频消息
				XM_PostMessage (XM_VIDEOSTOP, mod, 0);
			}
			else //if(Close_Audio_Sound)
			{
			    if(mod & XMKEY_REPEAT) {
					XM_PostMessage (XMKEY_REPEAT,  key, mod );
				}else if(mod & XMKEY_LONGTIME) {
					XM_PostMessage (XMKEY_LONGTIME,  key, mod );
				}else {
					XM_PostMessage (XM_KEYDOWN,  key, mod );
					//XM_PostMessage ((BYTE) ((mod & XMKEY_PRESSED) ? XM_KEYDOWN : XM_KEYUP),  key, mod );
				}
				//XM_PostMessage ((BYTE) ((mod & XMKEY_PRESSED) ? XM_KEYDOWN : XM_KEYUP),  key, mod );
			}
		}
	}

	// 触摸事件处理
	if(XM_TouchDriverPoll())
	{
		HasEvent = 1;
		while(XM_TouchDriverGetEvent (&point, &type))
		{
			if(type == TOUCH_TYPE_DOWN)
			{
				XM_PostMessage (XM_TOUCHDOWN,  0, point );
			}
			else if(type == TOUCH_TYPE_UP)
			{
				XM_PostMessage (XM_TOUCHUP,  0, point );
			}
			else if(type == TOUCH_TYPE_MOVE)
			{
				XM_PostMessage (XM_TOUCHMOVE,  0, point );
			}
		}
	}

	// 定时器事件处理
	if(XM_TimerDriverPoll())
	{
		HasEvent = 1;
		XM_TimerDriverDispatchEvent ();	// 检查并派发定时器消息到消息队列
	}
	// 处理串口输入事件
	if(XM_UartDriverPoll())
	{
		if(XM_UartDriverGetEvent (&key, &mod))
		{
			if(key == VK_AP_SYSTEM_EVENT)
				goto system_event_process;
			else
			{
				XM_PostMessage ((BYTE) ((mod & XMKEY_PRESSED) ? XM_KEYDOWN : XM_KEYUP),  key, mod );
				HasEvent = 1;
			}
		}
	}

	//return;
	//if(!HasEvent)
	//	XM_Sleep (1);
	//return;

	if(bBlockCaller && !HasEvent)
	{
		// 进入待机低功耗模式
		XM_idle ();
	}

}

static XMBOOL _XM_PeekMessage(XMMSG *lpmsg,	BYTE bMsgFilterMin, BYTE bMsgFilterMax, XMBOOL bBlockCaller)
{
	XMMSG* msg;
	kmsg_t kmsg;
	XMBOOL rescan = 1;

	// 检查队列中的消息
	if(queue_empty (&mcb_post_msg))
	{
		// 消息队列为空
		PollHardwareEvent (bBlockCaller);
		rescan = 0;
	}

	if(queue_empty (&mcb_post_msg))
		return 0;

rescan_message:
	kmsg = (kmsg_t) queue_next (&mcb_post_msg);
	while(kmsg && ((queue_s *)kmsg) != &mcb_post_msg)
	{
		msg = &kmsg->msg;
		if(bMsgFilterMin == 0 && bMsgFilterMax == 0)
		{
			// 不过滤任何消息
		}
		else
		{
			// 执行消息过滤
			if(msg->message >= bMsgFilterMin && msg->message <= bMsgFilterMax)
			{
				// 找到匹配消息
			}
			else
			{
				// 处理下一个消息
				kmsg = (kmsg_t) queue_next ((queue_s *)kmsg);
				continue;
			}
		}

		// 复制消息
		if(lpmsg)
		{
			lpmsg->wp = msg->wp;
			lpmsg->message = msg->message;
			lpmsg->lp = msg->lp;
			lpmsg->option = msg->option;
		}

		// 删除队列中的消息
		queue_delete ((queue_s *)kmsg);	// remove this kmsg item from kmsg queue
		del_kmsg (kmsg);

		return 1;
	}

	// 若消息为空，扫描硬件事件()
	PollHardwareEvent (bBlockCaller);

	if(rescan)
	{
		rescan = 0;
		goto rescan_message;	// 扫描MwSelect引起的硬件时间
	}
	return 0;
}

// 检索指定的消息。
// msg可为NULL，即删除指定的消息
// bMsgFilterMin = 0 && bMsgFilterMax = 0表示检索所有消息
// 成功返回TRUE，失败返回FALSE
XMBOOL 	XM_PeekMessage(XMMSG *msg,	BYTE bMsgFilterMin, BYTE bMsgFilterMax)
{
	return _XM_PeekMessage (msg, bMsgFilterMin, bMsgFilterMax, 0);
}

#if defined(PCVER)
static DWORD message_ticket = 0;

#if defined (__cplusplus)
	extern "C"{
#endif

DWORD xm_message_cycle = 25;		// 250毫秒发送一次测试按键


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif

XMBOOL	XM_GetMessage (XMMSG *msg)
{
	while(msg)
	{
		if(_XM_PeekMessage (msg, 0, 0, TRUE))
		{
			break;
		}

#if defined(PCVER)
		XM_Sleep (10);
		// PC测试接口，获取测试按键
		message_ticket ++;
		if( (message_ticket % xm_message_cycle) == 0)
		{
			XMTESTERMESSAGE Message;

			// 检查自动测试消息
			if(XM_TesterSendCommand (TESTER_CMD_MESSAGE, &Message))
			{
				// 获得的消息发送给当前窗口
				if(Message.wParam >= 0xF0 && Message.wParam <= 0xF2)	// 卡插入/拔出模拟按键
				{
					if(Message.message == XM_KEYDOWN)
					{
						msg->message = XM_SYSTEMEVENT;
						msg->wp = (WORD)(Message.wParam - 0xF0);
						msg->lp = 0;
						msg->option = XMMSG_OPTION_SYSTEMEVENT_DEFAULT_PROCESS;
						return 1;
					}
					else
					{
						// 忽略
						continue;
					}

				}
				else
				{
					msg->message = Message.message;
					msg->wp = Message.wParam;
					msg->lp = Message.lParam;
				}
				break;
			}
		}
#endif
		// 无任何消息处理
		// XM_Sleep (50);
	}

	// PC测试接口，记录按键事件
#if defined(PCVER)
	{
		BYTE modifier = (BYTE)(msg->lp);
		if(msg->message == XM_KEYDOWN && modifier & XMKEY_STROKE)
		{
			XMTESTERRECORD TesterRecord;

			TesterRecord.key = msg->wp;
			TesterRecord.reserved = modifier & (~XMKEY_STROKE);
			XM_TesterSendCommand (TESTER_CMD_RECORD, &TesterRecord);
		}
		else if(msg->message == XM_SYSTEMEVENT)
		{
			XMTESTERRECORD TesterRecord;

			TesterRecord.key = 0xF0 + msg->wp;
			TesterRecord.reserved = 0;
			XM_TesterSendCommand (TESTER_CMD_RECORD, &TesterRecord);

		}
	}
#endif

	return (XMBOOL)(msg->message != XM_QUIT);
}

// 派发消息
XMBOOL	XM_DispatchMessage (XMMSG *msg)
{
	HANDLE hWnd;
	XMHWND *pWnd;
	HWND_NODE *node;

	// 当存在视窗切换操作时(PUSH、PULL或JUMP), 禁止外部消息派发
	if(pullWnd || pushWnd || jumpWnd)
		return 0;

	if(window_count == 0)
	{
#ifdef _WINDOWS
		XM_ASSERT (0);
#endif
		return 0;
	}

	// 检查是否存在递归调用消息回调函数
	node = &window_stack[window_count - 1];
	if(node->flag & HWND_DISPATCH)
	{
#ifdef _WINDOWS
		// 严重错误。不允许嵌套调用
		XM_ASSERT (0);
#endif
		return 0;
	}

	hWnd = node->hwnd;
	if(hWnd == 0)
	{
		XM_ASSERT (0);
		return 0;
	}

	// 标记消息派发中，用于检查是否存在递归消息派发
	node->flag = (BYTE)(node->flag | HWND_DISPATCH);

	// 获取视窗结构的指针
	pWnd = ADDRESS_OF_HANDLE (hWnd);
	XM_ASSERT (pWnd);

	// 发送视窗退出消息
	(*pWnd->lpfnWndProc) (msg);

	// 清除派发中标志
	node->flag = (BYTE)(node->flag & ~HWND_DISPATCH);

	return 1;
}

void XM_FlushMessage (void)
{
	kmsg_t kmsg, next;

	// 检查消息队列是否为空
	if(!queue_empty (&mcb_post_msg))
	{
		// 遍历队列中的消息
		kmsg = (kmsg_t)queue_next (&mcb_post_msg);
		while(kmsg != (kmsg_t)&mcb_post_msg)
		{
			next = (kmsg_t)queue_next ((queue_s *)kmsg);
			// 删除该消息，将新消息加入到队列尾部
			queue_delete ((queue_s *)kmsg);
			// 将消息单元释放
			del_kmsg (kmsg);
			kmsg = next;
		}
	}
}

// 删除队列中所有非COMMAND消息
void XM_FlushMessageExcludeCommandAndSystemEvent (void)
{
	kmsg_t kmsg, next;

	// 检查消息队列是否为空
	if(!queue_empty (&mcb_post_msg))
	{
		// 遍历队列中的消息
		kmsg = (kmsg_t)queue_next (&mcb_post_msg);
		while(kmsg != (kmsg_t)&mcb_post_msg)
		{
			next = (kmsg_t)queue_next ((queue_s *)kmsg);
			if(kmsg->msg.message != XM_COMMAND && kmsg->msg.message != XM_SYSTEMEVENT)
			{
				// 删除该消息，将新消息加入到队列尾部
				queue_delete ((queue_s *)kmsg);
				// 将消息单元释放
				del_kmsg (kmsg);
			}
			kmsg = next;
		}
	}
}

// 终止系统事件消息的缺省处理
VOID XM_BreakSystemEventDefaultProcess (XMMSG *msg)
{
	if(msg == NULL)
		return;
	if(msg->message != XM_SYSTEMEVENT)
		return;
	msg->option &= ~XMMSG_OPTION_SYSTEMEVENT_DEFAULT_PROCESS;
}


// 设置窗口的私有数据
XMBOOL XM_SetWindowPrivateData (HANDLE hWnd, void *PrivateData)
{
	int i;

	for (i = 0; i < window_count; i++)
	{
		if(window_stack[i].hwnd == hWnd)
		{
			window_stack[i].PrivateData = PrivateData;
			return 1;
		}
	}

	return 0;
}

// 获取窗口的私有数据
void *	XM_GetWindowPrivateData (HANDLE hWnd)
{
	int i;

	for (i = 0; i < window_count; i++)
	{
		if(window_stack[i].hwnd == hWnd)
		{
			return window_stack[i].PrivateData;
		}
	}

	return NULL;
}


// 使整个窗口无效。系统投递XM_PAINT消息到消息队列尾部
XMBOOL XM_InvalidateWindow (void)
{
	HWND_NODE *node;
	if(window_count == 0)
	{
#ifdef _WINDOWS
		XM_ASSERT (0);
#endif
		return 0;
	}
	node = &window_stack[window_count - 1];
	node->flag |= HWND_DIRTY;
	return XM_PostMessage (XM_PAINT, 0, 0);
}

XMBOOL XM_InvalidateWidget (BYTE bWidgetIndex)
{
	HWND_NODE *node;
	if(window_count == 0)
	{
#ifdef _WINDOWS
		XM_ASSERT (0);
#endif
		return 0;
	}
	node = &window_stack[window_count - 1];
	if(bWidgetIndex >= node->cbWidget)
		return 0;
	node->lpWidgetFlag[bWidgetIndex] |= WDGT_DIRTY;
	return XM_PostMessage (XM_PAINT, 0, 0);
}

// 系统检查消息队列中的XM_PAINT消息。若存在，将其移至消息队列首部，以便在下一个消息循环中XM_PAINT消息被处理
XMBOOL XM_UpdateWindow (void)
{
	XMMSG msg;
	if(XM_PeekMessage (&msg, XM_PAINT, XM_PAINT))
	{
		return XM_InsertMessage (msg.message, msg.wp, msg.lp);
	}
	return 0;
}

static XMBOOL IsValidWindow (HANDLE hWnd)
{
	XMHWND *pWnd;
	XMBOOL valid = 0;

	pWnd = ADDRESS_OF_HANDLE (hWnd);

	while(pWnd)
	{
		if(pWnd->lpfnWndProc == NULL)
			break;
		if(pWnd->cbWidget && pWnd->lpWidget == NULL)
			break;
		if(pWnd->cbWidget == 0 && pWnd->lpWidget != NULL)
			break;

		valid = 1;
		break;
	}
	return valid;
}

// 在压入新的视窗时，同时指定新窗口的定制信息
// 定制信息(dwCustomData)通过XM_ENTER消息的lp参数传入
XMBOOL	XM_PushWindowEx	(HANDLE hWnd, DWORD dwCustomData)
{
	XMHWND *pWnd;
	if(hWnd != XMHWND_HANDLE(Desktop) && XM_GetFmlDeviceCap(DEVCAP_SCREEN) == DEVCAP_SCREEN_DISCONNECT)
		return 0;

	// 不允许窗口压栈/窗口弹栈/窗口跳转嵌套
	if(jumpWnd || pushWnd || pullWnd)
	{
		XM_printf ("XM_PushWindowEx NG, jumpWnd or pushWnd or pullWnd is not empty\n");
		return 0;
	}

	if(!IsValidWindow(hWnd))
		return 0;

	// 检查视窗数量
	if(window_count >= MAX_HWND_STACK)
	{
		return 0;
	}

	// 获取视窗结构的指针
	pWnd = ADDRESS_OF_HANDLE (hWnd);

	if(pWnd->cbWidget)
	{
#ifdef _WINDOWS
		XM_ASSERT (pWnd->cbWidget <= MAX_STACK_WIDGET_COUNT);
#endif
		// 视窗有定义Widget控件，检查是否有足够的控件标志用于分配
		if( (widget_count + pWnd->cbWidget) > MAX_STACK_WIDGET_COUNT )
		{
			// 系统无足够资源用于新的视窗
			return 0;
		}
	}

	// 记录压入视窗
	pushWnd = hWnd;
	customData = dwCustomData;

	return 1;
}

// XM_PushWindow
// 在当前视窗上压入新的视窗。应用于需要保存其过程且可以逐级返回的情形。
// 执行Push操作后，消息队列被清空
// 桌面不需要压入到栈中。当栈为空时，自动将桌面压入到栈顶。
// 如字典应用中，检索初始界面-->检索结果界面-->字典正文界面-->正文属性界面，其视窗栈表示如下
//    栈顶-->          正文属性界面
//                     字典正文界面
//                     检索结果界面
//    栈底-->          检索初始界面
XMBOOL	XM_PushWindow	(HANDLE hWnd)
{
	return XM_PushWindowEx (hWnd, HWND_CUSTOM_DEFAULT);
}

// XM_PullWindow
// 1) hWnd = 0, 弹出栈顶视窗,用于返回到前一个视窗。
//  如从 “正文属性界面” 返回到 “字典正文界面”
//
// 2) hWnd != 0, 循环弹出栈顶视窗，直到找到指定的视窗(hWnd)，并将该视窗置为栈顶视窗
// 如输出一个字符，导致从 “正文属性界面” 返回到 “检索初始界面”
XMBOOL XM_PullWindow	(HANDLE hWnd)
{
	HWND_NODE *node;
	BYTE i;

	if(XM_GetFmlDeviceCap(DEVCAP_SCREEN) == DEVCAP_SCREEN_DISCONNECT)
		return 0;

	// 不允许窗口压栈/窗口弹栈/窗口跳转嵌套
	if(jumpWnd || pushWnd || pullWnd)
	{
		XM_printf ("XM_PullWindow NG, jumpWnd or pushWnd or pullWnd is not empty\n");
		return 0;
	}

	// 检查视窗栈是否为空
	if(window_count == 0)
	{
#ifdef _WINDOWS
		XM_ASSERT (0);
#endif
		return 0;
	}

	if(hWnd == 0)
	{
		// 弹出栈顶(当前)窗口
		if(window_count == 1)
		{
			// 栈中只有一个视窗
			// 切换到桌面视窗，将桌面压入
			// pushWnd = XMHWND_HANDLE(Desktop);
			// 将桌面压入时,原有视窗被继续压入, 并没有弹出. 改为Jump方式, 将当前视窗彻底弹出并释放资源
			jumpWnd = XMHWND_HANDLE(Desktop);
			return 1;
		}

		// 记录弹出的视窗
		pullWnd = window_stack[window_count-2].hwnd;
		return 1;
	}

	// 扫描需要弹出的视窗是否在视窗栈中
	node = &window_stack[window_count-1];

	i = window_count;
	while (i > 0)
	{
		if(node->hwnd == hWnd)
		{
			pullWnd = hWnd;
			return 1;
		}
		i --;
		node --;
	}

	return 0;
}

// 将视窗栈中所有视窗清除，然后跳到新的视窗。一般用于功能性跳转使用
// 如上面的视窗栈情况下，按下“系统时间”功能键，会直接调用XM_WndGoto，跳转到“系统时间”。
XMBOOL	XM_JumpWindow	(HANDLE hWnd)
{
	return XM_JumpWindowEx (hWnd, HWND_CUSTOM_DEFAULT, XM_JUMP_POPDESKTOP);
}

// XM_JumpWindow
// 将视窗栈中栈顶视窗或所有视窗清除，然后跳到新的视窗。一般用于功能性跳转使用
// 如上面的视窗栈情况下，按下“系统时间”功能键，会直接调用XM_WndGoto，跳转到“系统时间”。
//	XM_JUMP_POPDEFAULT = 0,			//	将视窗栈中所有视窗清除(除去桌面)，然后跳到新的视窗
//	XM_JUMP_POPDESKTOP,				// 将视窗栈中所有视窗清除(包括桌面)，然后跳到新的视窗
//	XM_JUMP_POPTOPVIEW,				// 将视窗栈的栈顶视窗清除，然后跳到新的视窗
XMBOOL	XM_JumpWindowEx	(HANDLE hWnd, DWORD dwCustomData, XM_JUMP_TYPE JumpType)
{
	XMHWND *pWnd;
	XMBOOL ret = 1;

	if(XM_GetFmlDeviceCap(DEVCAP_SCREEN) == DEVCAP_SCREEN_DISCONNECT)
		return 0;

	// 不允许窗口压栈/窗口弹栈/窗口跳转嵌套
	if(jumpWnd || pushWnd || pullWnd)
	{
		XM_printf ("XM_JumpWindowEx NG, jumpWnd or pushWnd or pullWnd is not empty\n");
		return 0;
	}

	// 检查窗口句柄
	if(hWnd == NULL)
		return 0;
	if(!IsValidWindow (hWnd))
		return 0;

	jump_type = JumpType;

	// 获取视窗结构的指针
	pWnd = ADDRESS_OF_HANDLE (hWnd);
	// 检查Widget标记
	if(pWnd->cbWidget > MAX_STACK_WIDGET_COUNT)
	{
		// 系统错误
		XM_ASSERT (0);

		ret = 0;
	}

	if(ret == 0)
		return 0;

	jumpWnd = hWnd;
	customData = dwCustomData;

	return 1;
}


// 将窗口(视窗或控件)坐标转换为屏幕坐标
XMBOOL XM_ClientToScreen (HANDLE hWnd, XMPOINT *lpPoint)
{
	XMHWND *pWnd;

	if(hWnd == 0 || lpPoint == NULL)
		return 0;

	// 获取窗口结构的指针
	pWnd = ADDRESS_OF_HANDLE (hWnd);

	lpPoint->x = (XMCOORD)(lpPoint->x + pWnd->view_x);
	lpPoint->y = (XMCOORD)(lpPoint->y + pWnd->view_y);

	return 1;
}

// 将屏幕坐标转换为窗口(视窗或控件)坐标
XMBOOL XM_ScreenToClient (HANDLE hWnd, XMPOINT *lpPoint)
{
	XMHWND *pWnd;

	if(hWnd == 0 || lpPoint == NULL)
		return 0;

	// 获取窗口结构的指针
	pWnd = ADDRESS_OF_HANDLE (hWnd);

	lpPoint->x = (XMCOORD)(lpPoint->x - pWnd->view_x);
	lpPoint->y = (XMCOORD)(lpPoint->y - pWnd->view_y);

	return 1;
}

// 获取窗口(视窗或控件)的位置信息
XMBOOL XM_GetWindowRect (HANDLE hWnd, XMRECT* lpRect)
{
	XMHWND *pWnd;

	if(hWnd == 0 || lpRect == NULL)
		return 0;

	// 获取窗口结构的指针
	pWnd = ADDRESS_OF_HANDLE (hWnd);
	lpRect->left = pWnd->view_x;
	lpRect->top = pWnd->view_y;
	lpRect->right = (XMCOORD)(lpRect->left + pWnd->view_cx - 1);
	lpRect->bottom = (XMCOORD)(lpRect->top + pWnd->view_cy - 1);
	return 1;
}

// 设置窗口实例的位置(相对显示区原点)
XMBOOL XM_SetWindowPos (HANDLE hWnd,
								XMCOORD x, XMCOORD y,			// 屏幕坐标
								XMCOORD cx, XMCOORD cy
								)
{
	XMRECT rectDesktop;
	XMHWND *pWnd = ADDRESS_OF_HANDLE(hWnd);

	if(hWnd == 0)
		return 0;

	// 检查坐标
	XM_GetDesktopRect (&rectDesktop);
	if(x < rectDesktop.left)
		return 0;
	if(x >= rectDesktop.right)
		return 0;
	if(y < rectDesktop.top)
		return 0;
	if(y >= rectDesktop.bottom)
		return 0;
	if((x + cx - 1) > rectDesktop.right)
		return 0;
	if((y + cy - 1) > rectDesktop.bottom)
		return 0;

	// 获取窗口结构的指针
	pWnd->view_x = x;
	pWnd->view_y = y;
	pWnd->view_cx = cx;
	pWnd->view_cy = cy;

	XM_InvalidateWindow ();

	return 1;
}

// 获取当前视窗(即栈顶视窗)的唯一ID
// 当栈中存在多个同一视窗句柄的视窗时，可区分每个视窗。
BYTE	XM_GetWindowID	(VOID)
{
	if(window_count == 0)
	{
		return (BYTE)(-1);
	}

	return (BYTE)(window_count - 1);
}

// 缺省消息
VOID	XM_DefaultProc (XMMSG *msg)
{
}

HANDLE XM_GetDesktop (void)
{
	return XMHWND_HANDLE(Desktop);
}

VOID XM_GetDesktopRect (XMRECT *lpRect)
{
	XMHWND *pWnd = XMHWND_HANDLE(Desktop);

	if(lpRect)
	{
		//lpRect->left = 0;
		//lpRect->right = LCD_XDOTS - 1;
		//lpRect->top = 0;
		//lpRect->bottom = LCD_YDOTS - 1;

		lpRect->left = pWnd->view_x;
		lpRect->right = pWnd->view_x + pWnd->view_cx - 1;
		lpRect->top = pWnd->view_y;
		lpRect->bottom = pWnd->view_y + pWnd->view_cy - 1;
	}
}

// lprc矩形缩小(dx dy 这2个参数类型是unsigned char )
XMBOOL XM_InflateRect (XMRECT* lprc, XMCOORD dx, XMCOORD dy)
{
	if(lprc)
	{
		lprc->left = (XMCOORD)(lprc->left + dx);
		lprc->top = (XMCOORD)(lprc->top + dy);
		lprc->right = (XMCOORD)(lprc->right - dx);
		lprc->bottom = (XMCOORD)(lprc->bottom - dy);
		return 1;
	}
	else
	{
		return 0;
	}
}

XMBOOL XM_OffsetRect (XMRECT* lprc, XMCOORD dx, XMCOORD dy)
{
	if(lprc)
	{
		lprc->left = (XMCOORD)(lprc->left + dx);
		lprc->right = (XMCOORD)(lprc->right + dx);
		lprc->top = (XMCOORD)(lprc->top + dy);
		lprc->bottom = (XMCOORD)(lprc->bottom + dy);
		return 1;
	}
	else
	{
		return 0;
	}
}

XMBOOL XM_SetRect (XMRECT * lprc, XMCOORD xLeft, XMCOORD yTop, XMCOORD xRight, XMCOORD yBottom)
{
	if(lprc)
	{
		lprc->left = xLeft;
		lprc->top = yTop;
		lprc->right = xRight;
		lprc->bottom = yBottom;
		return 1;
	}
	else
	{
		return 0;
	}
}

XMBOOL XM_SetWidgetFlag (BYTE bWidgetIndex, BYTE bWidgetFlag)
{
	HWND_NODE *node;

	node = get_node();
	if(node == NULL)
		return 0;

	// 检查控件索引是否有效
	if(bWidgetIndex >= node->cbWidget)
		return 0;

	node->lpWidgetFlag[bWidgetIndex] = bWidgetFlag;
	XM_InvalidateWindow ();
	return 1;
}

BYTE XM_GetWidgetFlag (BYTE bWidgetIndex)
{
	HWND_NODE *node;

	node = get_node();
	if(node == NULL)
		return 0;

	// 检查控件索引是否有效
	if(bWidgetIndex >= node->cbWidget)
		return 0;

	return node->lpWidgetFlag[bWidgetIndex];
}


XMBOOL XM_SetFocus (BYTE bWidgetIndex)
{
	HWND_NODE *node;
	BYTE i;
	BYTE OldFocus, NewFocus;

	node = get_node ();
	if(node == NULL)
		return 0;

	OldFocus = (BYTE)(-1);
	NewFocus = (BYTE)(-1);

	for (i = 0; i < node->cbWidget; i++)
	{
		if(node->lpWidgetFlag[i] & WDGT_FOCUSED)
		{
			OldFocus = i;
			break;
		}
	}

	if(bWidgetIndex >= node->cbWidget)
		return 0;

	NewFocus = bWidgetIndex;

	// 聚焦没有修改
	if(OldFocus == NewFocus)
	{
		return 1;
	}

	if(OldFocus != (BYTE)(-1))
	{
		node->lpWidgetFlag[OldFocus] &= ~(WDGT_FOCUSED|WDGT_SELECT);
		XM_InvalidateWidget (OldFocus);
	}
	if(NewFocus != (BYTE)(-1))
	{
		node->lpWidgetFlag[NewFocus] |= WDGT_FOCUSED;
		XM_InvalidateWidget (NewFocus);
	}

	return 1;
}

BYTE XM_GetFocus (VOID)
{
	HWND_NODE *node;
	BYTE i;
	BYTE bFocus;

	bFocus = (BYTE)(-1);

	node = get_node ();
	if(node == NULL)
		return bFocus;

	for (i = 0; i < node->cbWidget; i++)
	{
		if(node->lpWidgetFlag[i] & WDGT_FOCUSED)
		{
			bFocus = i;
			break;
		}
	}

	return bFocus;
}

XMBOOL XM_SetSelect (BYTE bWidgetIndex, XMBOOL bSelect)
{
	HWND_NODE *node;

	node = get_node ();
	if(node == NULL)
		return 0;

	// 检查控件索引是否有效
	if(bWidgetIndex >= node->cbWidget)
		return 0;

	if(node->lpWidgetFlag[bWidgetIndex] & WDGT_SELECT)
	{
		if(bSelect == 0)		// 未选择
		{
			// 未选择
			node->lpWidgetFlag[bWidgetIndex] &= ~WDGT_SELECT;
			XM_InvalidateWidget (bWidgetIndex);
		}
	}
	else
	{
		if(bSelect)		// 已选择
		{
			node->lpWidgetFlag[bWidgetIndex] |= WDGT_SELECT;
			XM_InvalidateWidget (bWidgetIndex);
		}
	}

	return 1;
}

XMBOOL XM_SetEnable (BYTE bWidgetIndex, XMBOOL bEnable)
{
	HWND_NODE *node;

	node = get_node ();
	if(node == NULL)
		return 0;

	// 检查控件索引是否有效
	if(bWidgetIndex >= node->cbWidget)
		return 0;

	if(node->lpWidgetFlag[bWidgetIndex] & WDGT_ENABLE)
	{
		if(bEnable == 0)
		{
			// 未使能
			node->lpWidgetFlag[bWidgetIndex] &= ~WDGT_ENABLE;
			XM_InvalidateWidget (bWidgetIndex);
		}
	}
	else
	{
		if(bEnable)		// 使能
		{
			node->lpWidgetFlag[bWidgetIndex] |= WDGT_ENABLE;
			XM_InvalidateWidget (bWidgetIndex);
		}
	}

	return 1;
}

XMBOOL XM_SetVisual (BYTE bWidgetIndex, XMBOOL bVisual)
{
	HWND_NODE *node;

	node = get_node ();
	if(node == NULL)
		return 0;

	// 检查控件索引是否有效
	if(bWidgetIndex >= node->cbWidget)
		return 0;

	if(node->lpWidgetFlag[bWidgetIndex] & WDGT_VISUAL)
	{
		if(bVisual == 0)		// 隐藏
		{
			node->lpWidgetFlag[bWidgetIndex] &= ~WDGT_VISUAL;
			XM_InvalidateWindow ();
		}
	}
	else
	{
		if(bVisual)		// 显示
		{
			node->lpWidgetFlag[bWidgetIndex] |= WDGT_VISUAL;
			XM_InvalidateWindow ();
		}
	}

	return 1;
}

// 获取控件的视窗子控件索引
BYTE XM_GetWidgetIndex (const XMWDGT *pWidget)
{
	HWND_NODE *node;
	XMHWND *pWnd;
	BYTE i;

	node = get_node ();
	if(node == NULL)
		return (BYTE)(-1);

	pWnd = ADDRESS_OF_HANDLE (node->hwnd);
	for (i = 0; i < node->cbWidget; i++)
	{
		if( (pWnd->lpWidget + i) == pWidget )
			break;
	}

	if(i >= node->cbWidget)
		return (BYTE)(-1);
	return i;
}

// 获取当前视窗的句柄
HANDLE XM_GetWindow (VOID)
{
	HWND_NODE *node;
	node = get_node ();
	if(node == NULL)
		return NULL;

	return node->hwnd;
}

// 设置控件的用户数据
XMBOOL XM_SetWidgetUserData (BYTE bWidgetIndex, VOID *pUserData)
{
	HWND_NODE *node;

	node = get_node ();
	if(node == NULL)
		return 0;

	// 检查控件索引是否有效
	if(bWidgetIndex >= node->cbWidget)
		return 0;

	node->UserData[bWidgetIndex] = pUserData;
	return 1;
}

// 获取控件的用户数据
VOID *XM_GetWidgetUserData (BYTE bWidgetIndex)
{
	HWND_NODE *node;

	node = get_node ();
	if(node == NULL)
		return 0;

	// 检查控件索引是否有效
	if(bWidgetIndex >= node->cbWidget)
		return 0;

	return node->UserData[bWidgetIndex];
}

// 获取视窗的全局Alpha因子
unsigned char XM_GetWindowAlpha (HANDLE hWnd)
{
	HWND_NODE *node = get_node_from_hwnd (hWnd);
	XMHWND *pWnd = (XMHWND *)hWnd;
	if(pWnd == NULL)
		return 0;
	node = get_node_from_hwnd (hWnd);
	// 检查窗口是否实例化
	if(node)
		return node->alpha;
	else
		return pWnd->alpha;
}

// 设置视窗的全局Alpha因子
XMBOOL XM_SetWindowAlpha (HANDLE hWnd, unsigned char alpha)
{
	HWND_NODE *node;
	// 获取窗口的实例化对象句柄
	node = get_node_from_hwnd (hWnd);
	if(node)
	{
		node->alpha = alpha;
		return 1;
	}
	else
		return 0;
}

// 在指定的framebuffer上输出窗口hWnd, 用于动画过渡效果的输出
void _xm_paint_view (HANDLE hWnd, xm_osd_framebuffer_t framebuffer)
{
	XMMSG msg;
	HWND_NODE *node;
	BYTE	i;
	XMHWND *pWnd;
	xm_osd_framebuffer_t old_framebuffer;

	if(framebuffer == NULL || hWnd == NULL)
		return;

	node = NULL;
	for (i = 0; i < window_count; i++)
	{
		if(window_stack[i].hwnd == hWnd)
		{
			node = &window_stack[i];
			break;
		}
	}
	if(node == NULL)
		return;

	// 过滤消息队列中存在的PAINT消息
	XM_PeekMessage (&msg, XM_PAINT, XM_PAINT);
	// 屏蔽视窗脏标志
	node->flag &= ~HWND_DIRTY;

	// 标记消息派发中，用于检查是否存在递归消息派发
	//node->flag = HWND_DISPATCH;

	old_framebuffer = node->framebuffer;
	node->framebuffer = framebuffer;

	msg.message = XM_PAINT;
	msg.wp = 0;
	msg.lp = 0;

	// 获取视窗结构的指针
	pWnd = ADDRESS_OF_HANDLE (hWnd);

	(*pWnd->lpfnWndProc) (&msg);

	// 向子控件发送XM_PAINT消息
	for (i = 0; i < pWnd->cbWidget; i ++)
	{
		if(WIDGET_IS_VISUAL(node->lpWidgetFlag[i]))
			XM_WidgetProc (pWnd->lpWidget + i, node->lpWidgetFlag[i], node->UserData[i], &msg);
	}

	// node->framebuffer = NULL;
	node->framebuffer = old_framebuffer;

	//node->flag = (BYTE)(node->flag & ~HWND_DISPATCH);
}

// 使能视窗创建、关闭时的动画效果
XMBOOL XM_EnableViewAnimate (HANDLE hWnd)
{
	HWND_NODE *node = get_node_from_hwnd (hWnd);
	if(node == NULL)
		return 0;
	node->flag |= HWND_ANIMATE;
	return 1;
}

// 禁止视窗创建、关闭时的动画效果
XMBOOL XM_DisableViewAnimate (HANDLE hWnd)
{
	HWND_NODE *node = get_node_from_hwnd (hWnd);
	if(node == NULL)
		return 0;
	node->flag &= ~HWND_ANIMATE;
	return 1;
}


// 根据桌面与基准屏之间的关系计算视窗对齐方式及缩放因子
static void XM_init_view_reference_coordinate (void)
{
	XMHWND *pDesktopWnd = XMHWND_HANDLE(Desktop);
	unsigned int osd_width, osd_height;
	view_default_align = XM_VIEW_ALIGN_HORZ_CENTRE | XM_VIEW_ALIGN_VERT_CENTRE;
	view_default_horz_ratio = 1.0;
	view_default_vert_ratio = 1.0;

	if(pDesktopWnd->_x < 0 || (pDesktopWnd->_x + pDesktopWnd->_cx) > LCD_XDOTS)
		return;
	if(pDesktopWnd->_y < 0 || (pDesktopWnd->_y + pDesktopWnd->_cy) > LCD_YDOTS)
		return;

	if(pDesktopWnd->_x == 0)
	{
		// 左侧相连
		if(pDesktopWnd->_cx == LCD_XDOTS)	// 水平居中，水平缩放比例为1.0
		{
			view_default_align = XM_VIEW_ALIGN_HORZ_CENTRE;
			view_default_horz_ratio = 1.0;
		}
		else
		{
			view_default_align = XM_VIEW_ALIGN_HORZ_LEFT;	// 左对齐
			view_default_horz_ratio = (float)(((float)pDesktopWnd->_cx) / LCD_XDOTS);
		}
	}
	else if( (pDesktopWnd->_x + pDesktopWnd->_cx) == LCD_XDOTS )
	{
		// 右侧相连, 右对齐
		view_default_align = XM_VIEW_ALIGN_HORZ_RIGHT;
		view_default_horz_ratio = (float)(((float)pDesktopWnd->_cx + pDesktopWnd->_x) / LCD_XDOTS);
	}
	else
	{
		// 比例对齐
		view_default_align = XM_VIEW_ALIGN_HORZ_SCALE;
		view_default_horz_ratio = (float)(((float)pDesktopWnd->_cx + pDesktopWnd->_x) / LCD_XDOTS);
	}

	if(pDesktopWnd->_y == 0)
	{
		// 顶部相连
		if(pDesktopWnd->_cy == LCD_YDOTS)	// 垂直居中，垂直缩放比例为1.0
		{
			view_default_align |= XM_VIEW_ALIGN_VERT_CENTRE;
			view_default_vert_ratio = 1.0;
		}
		else
		{
			view_default_align |= XM_VIEW_ALIGN_VERT_TOP;	// 向上对齐
			view_default_vert_ratio = (float)(((float)pDesktopWnd->_cy) / LCD_YDOTS);
		}
	}
	else if( (pDesktopWnd->_y + pDesktopWnd->_cy) == LCD_YDOTS )
	{
		// 底部相连, 向下对齐
		view_default_align |= XM_VIEW_ALIGN_VERT_BOTTOM;
		view_default_vert_ratio = (float)(((float)pDesktopWnd->_cy + pDesktopWnd->_y) / LCD_YDOTS);
	}
	else
	{
		// 比例对齐
		view_default_align |= XM_VIEW_ALIGN_VERT_SCALE;
		view_default_vert_ratio = (float)(((float)pDesktopWnd->_cy + pDesktopWnd->_y) / LCD_YDOTS);
	}

	// 计算目标系统与基准屏之间的比例
	osd_width = XM_lcdc_osd_get_width (0, XM_OSD_LAYER_1);
	osd_height = XM_lcdc_osd_get_height (0, XM_OSD_LAYER_1);


	view_default_horz_ratio = view_default_horz_ratio * osd_width / LCD_XDOTS;
	view_default_vert_ratio = view_default_vert_ratio * osd_height / LCD_YDOTS;

	// 计算桌面视图
	pDesktopWnd->view_x = (XMCOORD)(pDesktopWnd->_x * view_default_horz_ratio + 0.5);
	pDesktopWnd->view_cx = (XMCOORD)(pDesktopWnd->_cx * view_default_horz_ratio + 0.5);
	pDesktopWnd->view_y = (XMCOORD)(pDesktopWnd->_y * view_default_vert_ratio + 0.5);;
	pDesktopWnd->view_cy = (XMCOORD)(pDesktopWnd->_cy * view_default_vert_ratio + 0.5);
}

DWORD XM_MainLoop (VOID)
{
	XMMSG msg;
	HANDLE hWnd;
	XMHWND *pWnd;

	HWND_NODE *node;
	BYTE *flag;
	BYTE	i;
	BYTE bViewCloseAnimating;		// 关闭视窗时允许动画效果
	const XMWDGT *widget;
	DWORD dwExitCode = 0;

	XMBOOL IsMessageLoop = TRUE;

	XMHWND *pDesktopWnd = XMHWND_HANDLE(Desktop);
	HANDLE hTopWnd = NULL;
	UINT uTopWndAnimateDirection = XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP;

	// 系统主消息循环
	while (IsMessageLoop)
	{
		hWnd = 0;
		bViewCloseAnimating = 1;

		// 检查是否存在Jump视窗
		if(jumpWnd)
		{
			// 检查顶层窗口，是否是ALERT类型的视窗。
			// 1) 若是ALERT视窗，需完成ALERT视窗的关闭过程, 恢复ALERT视窗创建前的其他OSD层的控制参数
			// 2) 若是其他类型视窗, 则直接关闭。(因为其他类型的视窗创建过程中不存在对其他OSD层参数的修改)
			node = &window_stack[window_count - 1];
			if(node->flag & HWND_ANIMATE)
			{
				HANDLE hwnd = node->hwnd;
				if(((XMHWND *)ADDRESS_OF_HANDLE(hwnd))->type == HWND_ALERT)
				{
					// Alert强制为自顶而下
					XM_osd_layer_animate (
						XM_OSDLAYER_ANIMATE_MODE_ALERT_REMOVE,
						XM_OSDLAYER_MOVING_FROM_TOP_TO_BOTTOM,
						hwnd,
						NULL
						);
				}
			}

			// 将视窗栈中的所有视窗退栈，从栈顶开始遍历直到栈底
			node = &window_stack[window_count - 1];
			msg.message = XM_LEAVE;
			msg.wp = 0;
			msg.lp = 0;
			while(window_count)
			{
				hWnd =  node->hwnd;

				// 检查是否弹出桌面窗口
				if(hWnd == XMHWND_HANDLE(Desktop))
				{
					if(jump_type == XM_JUMP_POPDEFAULT)
					{
						// 不弹出桌面，退出
						break;
					}
				}

				// 标记消息派发中，用于检查是否存在递归消息派发
				node->flag = (BYTE)(node->flag | HWND_DISPATCH);

				// 获取视窗结构的指针
				pWnd = ADDRESS_OF_HANDLE (hWnd);

				// 检查子控件是否存在。若存在，向子控件发送退出消息
				for (i = 0; i < pWnd->cbWidget; i ++)
				{
					XM_WidgetProc (pWnd->lpWidget + i, node->lpWidgetFlag[i], node->UserData[i], &msg);
				}

				// 向主视窗发送视窗退出消息
				(*pWnd->lpfnWndProc) (&msg);

				XM_ASSERT (pWnd->cbWidget <= widget_count);
				// 释放视窗标记资源
				widget_count = (BYTE)(widget_count - pWnd->cbWidget);

				node->hwnd = 0;
				node->lpWidgetFlag = NULL;
				node->flag = 0;
				node->cbWidget = 0;
				node->UserData = 0;		// 将用户数据无效

				node --;

				window_count --;

				// 仅弹出栈顶视窗
				if(jump_type == XM_JUMP_POPTOPVIEW)
					break;

			}

			jump_type = XM_JUMP_POPDEFAULT;

			// 检查是否跳转到桌面(Desktop)
			if(jumpWnd == XMHWND_HANDLE(Desktop))
			{
				// 检查桌面窗口是否已创建(存在)
				if(window_count == 1 && window_stack[0].hwnd == jumpWnd)
				{
					XM_ASSERT (jumpWnd == node->hwnd);
					hWnd =  jumpWnd;
					jumpWnd = NULL;
					goto desktop_reenter;
				}
			}

			// 记录需要新建视窗
			hWnd = jumpWnd;

			// 清除JUMP视窗
			jumpWnd = 0;
		} // if(jumpWnd)
		// 检查是否存在PUSH视窗
		else if(pushWnd)
		{
			// 允许PUSH视窗时，视窗栈为空
			if(window_count > 0)
			{
				// 向栈顶视窗(当前视窗)发送XM_LEAVE消息(临时离开消息)
				msg.message = XM_LEAVE;
				msg.wp = 1;	// 临时离开
				msg.lp = 0;

				node = &window_stack[window_count - 1];
				hWnd = node->hwnd;

				// 检查窗口是否重入。若是，报警并返回
				if(pushWnd == hWnd)
				{
					XM_printf ("ERROR! Can't re-enter the same view (hwnd = 0x%08x)\n", hWnd);
					pushWnd = NULL;
					continue;
				}

				// 标记消息派发中，用于检查是否存在递归消息派发
				node->flag = (BYTE)(node->flag | HWND_DISPATCH);

				// 获取视窗结构的指针
				pWnd = ADDRESS_OF_HANDLE (hWnd);
				// 检查子控件是否存在。若存在，向子控件发送XM_LEAVE消息(临时离开消息)
				for (i = 0; i < pWnd->cbWidget; i ++)
				{
					XM_WidgetProc (pWnd->lpWidget + i, node->lpWidgetFlag[i], node->UserData[i], &msg);
				}

				// 向视窗发送XM_LEAVE消息(临时离开消息)
				(*pWnd->lpfnWndProc) (&msg);

				node->flag = (BYTE)(node->flag & ~HWND_DISPATCH);
			}

			// 记录需要新建视窗
			hWnd = pushWnd;

			// 清除PUSH视窗
			pushWnd = 0;
		}
		// 检查是否存在弹出视窗
		else if(pullWnd)
		{
			// 存在弹出视窗

			// 从栈顶开始遍历直到遇到匹配的视窗，并发送视窗退出消息
			node = &window_stack[window_count - 1];
			if(!(node->flag & HWND_ANIMATE))
			{
				bViewCloseAnimating = 0;
			}

			hTopWnd = node->hwnd;
			uTopWndAnimateDirection = node->animatingDirection;

			msg.message = XM_LEAVE;
			msg.wp = 0;
			msg.lp = 0;
			while(pullWnd)
			{
				hWnd =  node->hwnd;
				if(hWnd == pullWnd)
					break;

				// 标记消息派发中，用于检查是否存在递归消息派发
				node->flag = (BYTE)(node->flag | HWND_DISPATCH);

				// 获取视窗结构的指针
				pWnd = ADDRESS_OF_HANDLE (hWnd);
				// 检查子控件是否存在。若存在，向子控件发送退出消息
				for (i = 0; i < pWnd->cbWidget; i ++)
				{
					XM_WidgetProc (pWnd->lpWidget + i, node->lpWidgetFlag[i], node->UserData[i], &msg);
				}

				// 发送视窗退出消息
				(*pWnd->lpfnWndProc) (&msg);

				XM_ASSERT (pWnd->cbWidget <= widget_count);
				// 释放视窗标记资源
				widget_count = (BYTE)(widget_count - pWnd->cbWidget);

				// node->flag = (BYTE)(node->flag & ~HWND_DISPATCH);

				node->hwnd = 0;
				node->lpWidgetFlag = NULL;
				node->flag = 0;
				node->cbWidget = 0;
				node->UserData = 0;

				node --;

				window_count --;
			}

desktop_reenter:
			// 发送视窗重新进入消息
			XM_ASSERT (window_count > 0);

			// 向栈顶视窗(当前视窗)发送XM_ENTER消息(重新进入消息)
			msg.message = XM_ENTER;
			msg.wp = 1;	// 重新进入消息
			msg.lp = 0;

			// 标记消息派发中，用于检查是否存在递归消息派发
			node->flag = (BYTE)(node->flag | HWND_DISPATCH);

			// 获取视窗结构的指针
			pWnd = ADDRESS_OF_HANDLE (hWnd);
			// 向子控件发送XM_ENTER消息
			for (i = 0; i < pWnd->cbWidget; i ++)
			{
				XM_WidgetProc (pWnd->lpWidget + i, node->lpWidgetFlag[i], node->UserData[i], &msg);
			}

			// 向窗口发送XM_ENTER消息
			(*pWnd->lpfnWndProc) (&msg);


			if(	bViewCloseAnimating && node->flag & HWND_ANIMATE
				|| (hTopWnd && ((XMHWND *)ADDRESS_OF_HANDLE(hTopWnd))->type == HWND_ALERT)		// 顶层视窗为alert时强制开启动画
				)
			{
				if(	(hTopWnd && ((XMHWND *)ADDRESS_OF_HANDLE(hTopWnd))->type == HWND_ALERT)
					)
				{
					// Alert强制为自顶而下
					XM_osd_layer_animate (
						XM_OSDLAYER_ANIMATE_MODE_ALERT_REMOVE,
						XM_OSDLAYER_MOVING_FROM_TOP_TO_BOTTOM,
						hTopWnd,
						hWnd
						);
				}
				else if(hTopWnd && ((XMHWND *)ADDRESS_OF_HANDLE(hTopWnd))->type == HWND_EVENT)
				{
					// Event强制为自底而上移出
					XM_osd_layer_animate (
						XM_OSDLAYER_ANIMATE_MODE_EVENT_REMOVE,
						XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP,
						hTopWnd,
						hWnd
						);
				}
				else if(hWnd && hWnd == XMHWND_HANDLE(Desktop))		// 弹出窗口为桌面
				{
					XM_osd_layer_animate (
						XM_OSDLAYER_ANIMATE_MODE_VIEW_REMOVE,
						XM_OSDLAYER_MOVING_FROM_TOP_TO_BOTTOM,
						hTopWnd,
						hWnd
						);
				}
				else
				{
					XM_osd_layer_animate (
						XM_OSDLAYER_ANIMATE_MODE_VIEW_REMOVE,
						(uTopWndAnimateDirection == XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP) ?
							XM_OSDLAYER_MOVING_FROM_TOP_TO_BOTTOM : XM_OSDLAYER_MOVING_FROM_LEFT_TO_RIGHT,
						hTopWnd,
						hWnd);
				}

				uTopWndAnimateDirection = XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP;
			}
			else
			{
				int forced = 0;
				if(pWnd->type != HWND_VIEW)
				{
					node->framebuffer = XM_osd_framebuffer_create (
						XM_LCDC_CHANNEL_0,
						XM_OSD_LAYER_2,
						XM_lcdc_osd_get_ui_format (XM_LCDC_CHANNEL_0),
						pDesktopWnd->view_cx,
						pDesktopWnd->view_cy,
						hWnd,
						0,
						0
						);
					forced = 1;
				}
				else
				{
					// 位于OSD 2层，需要手工关闭该OSD层
					XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, (1 << XM_OSD_LAYER_2), (1 << XM_OSD_LAYER_2) );
					XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_2);
					XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, (1 << XM_OSD_LAYER_2), 0 );

					if(pWnd != pDesktopWnd)
					{
						node->framebuffer = XM_osd_framebuffer_create (
							XM_LCDC_CHANNEL_0,
							XM_OSD_LAYER_1,
							XM_lcdc_osd_get_ui_format (XM_LCDC_CHANNEL_0),
							pDesktopWnd->view_cx,
							pDesktopWnd->view_cy,
							hWnd,
							0,
							0
							);
					}
				}

				if(node->framebuffer)
				{
					// 发送XM_PAINT消息
					msg.message = XM_PAINT;
					msg.wp = 0;	// 重新进入消息
					msg.lp = 0;

					// 屏蔽视窗脏标志
					node->flag &= ~HWND_DIRTY;
					// 向窗口发送XM_PAINT消息
					(*pWnd->lpfnWndProc) (&msg);

					// 向子控件发送XM_PAINT消息
					for (i = 0; i < pWnd->cbWidget; i ++)
					{
						if(WIDGET_IS_VISUAL(node->lpWidgetFlag[i]))
							XM_WidgetProc (pWnd->lpWidget + i, node->lpWidgetFlag[i], node->UserData[i], &msg);
						node->lpWidgetFlag[i] &= ~WDGT_DIRTY;
					}
					XM_osd_framebuffer_close (node->framebuffer, forced);

					node->framebuffer = NULL;
				}

			}

			hTopWnd = NULL;

			node->flag = (BYTE)(node->flag & ~HWND_DISPATCH);

			// 清除PULL视窗
			pullWnd = 0;

			// 标记无新视窗需要创建
			hWnd = 0;
		}

		// 检查是否存在新建视窗
		if(hWnd)
		{
			node = &window_stack[window_count];

			// 获取视窗结构的指针
			pWnd = ADDRESS_OF_HANDLE (hWnd);

			// 将窗口初始化结构的坐标、尺寸信息、alpha因子复制到视窗节点中去，允许后续修改
			// 根据桌面视窗的定义坐标( _x, _y, _cx, _cy ) 与 基准屏的基准坐标 (0, 0, LCD_XDOTS, LCD_YDOTS) 之间的关系，
			// 计算每个视窗映射到目标投射系统之间的比例、对齐方式等
			pWnd->view_x = (XMCOORD)(pWnd->_x * view_default_horz_ratio + 0.5);
			pWnd->view_cx = (XMCOORD)(pWnd->_cx * view_default_horz_ratio + 0.5);
			pWnd->view_y = (XMCOORD)(pWnd->_y * view_default_vert_ratio + 0.5);;
			pWnd->view_cy = (XMCOORD)(pWnd->_cy * view_default_vert_ratio + 0.5);

			/*
			pWnd->view_x = pWnd->_x;
			pWnd->view_y = pWnd->_y;
			pWnd->view_cx = pWnd->_cx;
			pWnd->view_cy = pWnd->_cy;
			*/

			node->alpha = pWnd->alpha;

			if(pWnd->cbWidget)
			{
				// 视窗有定义Widget控件

				// 分配控件标志
				node->lpWidgetFlag = widget_flag + widget_count;
				// 复制初始标志
				widget = pWnd->lpWidget;

				flag = node->lpWidgetFlag;
				for (i = 0; i < pWnd->cbWidget; i ++)
				{
					*flag = widget->bFlag;
					flag ++;
					widget ++;
				}

				// 设置控件用户数据, 全部初始为空。
				// 视窗需在进入时为所有子控件设置用户数据，以便子控件进入时可以使用用户数据
				node->UserData = widget_userdata + widget_count;
				for (i = 0; i < pWnd->cbWidget; i ++)
				{
					node->UserData[i] = 0;
				}

				widget_count = (BYTE)(widget_count + pWnd->cbWidget);
			}
			else
			{
				node->lpWidgetFlag = 0;
				node->UserData = 0;
			}
			node->cbWidget = pWnd->cbWidget;

			node->hwnd = hWnd;
			node->flag = HWND_ANIMATE;		// 缺省支持动画效果
			node->PrivateData = NULL;

			window_count ++;

			// 清空消息队列中所有消息
			// XM_FlushMessage ();
			XM_FlushMessageExcludeCommandAndSystemEvent ();


			// 发送视窗进入(XM_ENTER)消息
			msg.message = XM_ENTER;
			msg.wp = 0;
			msg.lp = customData;
			// 自动将定制属性恢复为缺省属性
			customData = HWND_CUSTOM_DEFAULT;

			// 标记消息派发中，用于检查是否存在递归消息派发
			node->flag |= HWND_DISPATCH;

			(*pWnd->lpfnWndProc) (&msg);

			// 检查创建过程是否被终止。如跳转到其他窗口
			if(jumpWnd || pushWnd || pullWnd)
			{
				node->flag &= ~HWND_DISPATCH;

				// 保存视窗创建时的Animating方向。窗口摧毁时反方向移动
				node->animatingDirection = view_animating_direction;

				// 恢复系统缺省值
				view_animating_direction = XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP;
				continue;
			}

			// 检查子控件是否存在。若存在，向子控件发送进入(XM_ENTER)消息
			for (i = 0; i < pWnd->cbWidget; i ++)
			{
				XM_WidgetProc (pWnd->lpWidget + i, node->lpWidgetFlag[i], node->UserData[i], &msg);
			}

			// 保存视窗创建时的Animating方向。窗口摧毁时反方向移动
			node->animatingDirection = view_animating_direction;

			// 恢复系统缺省值
			view_animating_direction = XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP;

			// ************ 向主窗口发送XM_PAINT消息 ***************
			if(node->flag & HWND_ANIMATE)
			{
				// 支持动画效果
				if(pWnd->type == HWND_VIEW)
				{
					// 普通视窗
					XM_osd_layer_animate (
						XM_OSDLAYER_ANIMATE_MODE_VIEW_CREATE,
						node->animatingDirection,
						NULL,
						hWnd);
				}
				else if(pWnd->type == HWND_EVENT)
				{
					// 通知视窗，强制自顶向下滑入
					XM_osd_layer_animate (
						XM_OSDLAYER_ANIMATE_MODE_EVENT_CREATE,
						XM_OSDLAYER_MOVING_FROM_TOP_TO_BOTTOM,
						NULL,
						hWnd);
				}
				else
				{
					// Alert, 强制自下而上移动
					node->animatingDirection = XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP;
					XM_osd_layer_animate (
						XM_OSDLAYER_ANIMATE_MODE_ALERT_CREATE,
						node->animatingDirection,
						NULL,
						hWnd);
				}
			}
			else
			{
				// 非动画效果
				int forced = 0;
				if(pWnd->type != HWND_VIEW)
				{
					node->framebuffer = XM_osd_framebuffer_create (
						XM_LCDC_CHANNEL_0,
						XM_OSD_LAYER_2,
						XM_lcdc_osd_get_ui_format (XM_LCDC_CHANNEL_0),
						pDesktopWnd->view_cx,
						pDesktopWnd->view_cy,
						hWnd,
						0,
						0
						);
					forced = 1;
				}
				else
				{
					XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, (1 << XM_OSD_LAYER_2), (1 << XM_OSD_LAYER_2) );
					XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_2);
					XM_lcdc_osd_layer_set_lock (XM_LCDC_CHANNEL_0, (1 << XM_OSD_LAYER_2), 0 );

					if(pWnd != pDesktopWnd)
					{
						node->framebuffer = XM_osd_framebuffer_create (
							XM_LCDC_CHANNEL_0,
							XM_OSD_LAYER_1,
							XM_lcdc_osd_get_ui_format (XM_LCDC_CHANNEL_0),
							pDesktopWnd->view_cx,
							pDesktopWnd->view_cy,
							hWnd,
							0,
							0
							);
					}
				}

				if(node->framebuffer)
				{
					msg.message = XM_PAINT;
					msg.wp = 0;
					msg.lp = 0;

					(*pWnd->lpfnWndProc) (&msg);

					// 检查子控件是否存在。若存在，向子控件发送刷新消息
					for (i = 0; i < pWnd->cbWidget; i ++)
					{
						if(WIDGET_IS_VISUAL(node->lpWidgetFlag[i]))
							XM_WidgetProc (pWnd->lpWidget + i, node->lpWidgetFlag[i], node->UserData[i], &msg);
					}
					XM_osd_framebuffer_close (node->framebuffer, forced);

					node->framebuffer = NULL;
				}
			}

			node->flag = (BYTE)(node->flag & ~HWND_DISPATCH);

			// 检查是否存在新视窗创建/视窗弹出操作
			if(jumpWnd || pushWnd || pullWnd)
				continue;
		}
		else
		{
			// 检查视窗栈是否为空
			if(window_count == 0)
			{
				XM_ASSERT (widget_count == 0);
				XM_PushWindow (XMHWND_HANDLE(Desktop));
				continue;
			}
		}

		// 获取下一条消息
		if(!XM_GetMessage (&msg))
		{
			// 消息循环退出消息

			// 检查是否是分辨率切换导致的消息循环终止
			if(msg.lp == XMEXIT_CHANGE_RESOLUTION)
			{
				if(XM_GetFmlDeviceCap(DEVCAP_SCREEN) == DEVCAP_SCREEN_DISCONNECT)
				{
					// 无显示设备连接
					// 关闭所有(除去桌面)当前显示的视窗
					while(window_count > 0)
					{
						node = &window_stack[window_count - 1];
						hWnd = node->hwnd;
						// 获取视窗结构的指针
						pWnd = ADDRESS_OF_HANDLE (hWnd);
						if( pWnd == pDesktopWnd )
						{
							break;
						}
						else
						{
							XMMSG leave_msg;
							leave_msg.message = XM_LEAVE;
							leave_msg.wp = 0;
							leave_msg.lp = 0;
							// 标记消息派发中，用于检查是否存在递归消息派发
							node->flag = (BYTE)(node->flag | HWND_DISPATCH);
							// 检查子控件是否存在。若存在，向子控件发送退出消息
							for (i = 0; i < pWnd->cbWidget; i ++)
							{
								XM_WidgetProc (pWnd->lpWidget + i, node->lpWidgetFlag[i], node->UserData[i], &leave_msg);
							}
							// 发送视窗退出消息
							(*pWnd->lpfnWndProc) (&leave_msg);

							XM_ASSERT (pWnd->cbWidget <= widget_count);

							// 释放视窗标记资源
							widget_count = (BYTE)(widget_count - pWnd->cbWidget);
							node->hwnd = 0;
							node->lpWidgetFlag = NULL;
							node->flag = 0;
							node->cbWidget = 0;
							node->UserData = 0;

							node --;

							window_count --;
						}
					}
				}
				else
				{
					if(window_count > 0)
					{
						// 检查顶层视窗(TOP VIEW)是否是 HWND_EVENT 或者 HWND_ALERT 类型
						// 若是，将该顶层视窗(TOP VIEW)关闭, (发送 XM_LEAVE 消息)
						node = &window_stack[window_count - 1];
						hWnd = node->hwnd;
						// 获取视窗结构的指针
						pWnd = ADDRESS_OF_HANDLE (hWnd);
						if( /* pWnd->type == HWND_EVENT ||*/ pWnd->type == HWND_ALERT )
						{
							XMMSG leave_msg;
							leave_msg.message = XM_LEAVE;
							leave_msg.wp = 0;
							leave_msg.lp = 0;
							// 标记消息派发中，用于检查是否存在递归消息派发
							node->flag = (BYTE)(node->flag | HWND_DISPATCH);
							// 检查子控件是否存在。若存在，向子控件发送退出消息
							for (i = 0; i < pWnd->cbWidget; i ++)
							{
								XM_WidgetProc (pWnd->lpWidget + i, node->lpWidgetFlag[i], node->UserData[i], &leave_msg);
							}
							// 发送视窗退出消息
							(*pWnd->lpfnWndProc) (&leave_msg);

							XM_ASSERT (pWnd->cbWidget <= widget_count);

							// 释放视窗标记资源
							widget_count = (BYTE)(widget_count - pWnd->cbWidget);
							node->hwnd = 0;
							node->lpWidgetFlag = NULL;
							node->flag = 0;
							node->cbWidget = 0;
							node->UserData = 0;

							node --;

							window_count --;

							// OSD 2层关闭
							XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_2);
							// 将参数同步到IO寄存器并等待硬件同步完成
							HW_lcdc_set_osd_coef_syn (XM_LCDC_CHANNEL_0, (1 << XM_OSD_LAYER_2));
						}
					}
				}
			}

			dwExitCode = msg.lp;
			break;
		}

		// 派发消息
		// 获取栈顶视窗句柄
		node = &window_stack[window_count - 1];
		hWnd = node->hwnd;

		// 标记消息派发中，用于检查是否存在递归消息派发
		node->flag = (BYTE)(node->flag | HWND_DISPATCH);

		// 获取视窗结构的指针
		pWnd = ADDRESS_OF_HANDLE (hWnd);
		if(msg.message == XM_PAINT && pWnd != pDesktopWnd)
		{
			// PAINT消息。先发送给视窗，再发送给控件
			XMBOOL PaintAll = 0;
			if(	pWnd->type == HWND_VIEW
				||	pWnd->type == HWND_EVENT
				)
			{
				// 普通视窗
				node->framebuffer = XM_osd_framebuffer_create (
					XM_LCDC_CHANNEL_0,
					XM_OSD_LAYER_1,
					XM_lcdc_osd_get_ui_format (XM_LCDC_CHANNEL_0),
					pDesktopWnd->view_cx,
					pDesktopWnd->view_cy,
					hWnd,
					1,
					0
					);
			}
			else
			{
				// ALERT视窗
				node->framebuffer = XM_osd_framebuffer_create (
					XM_LCDC_CHANNEL_0,
					XM_OSD_LAYER_2,
					XM_lcdc_osd_get_ui_format (XM_LCDC_CHANNEL_0),
					pDesktopWnd->view_cx,
					pDesktopWnd->view_cy,
					hWnd,
					1,
					0
					);
			}


			if(node->flag & HWND_DIRTY)
			{
				PaintAll = 1;
				node->flag &= ~HWND_DIRTY;
			}

			if(PaintAll)
			{
				(*pWnd->lpfnWndProc) (&msg);
			}

			// 如果存在子控件，将消息派发到子控件
			for (i = 0; i < pWnd->cbWidget; i ++)
			{
				// 检查控件是否可视且存在脏区域
				if(WIDGET_IS_VISUAL(node->lpWidgetFlag[i]))
				{
					if( PaintAll || WIDGET_IS_DIRTY(node->lpWidgetFlag[i]))
					{
						XM_WidgetProc (pWnd->lpWidget + i, node->lpWidgetFlag[i], node->UserData[i], &msg);
						node->lpWidgetFlag[i] &= ~WDGT_DIRTY;
					}
				}
			}

			if(node->framebuffer)
				XM_osd_framebuffer_close (node->framebuffer, 0);

			node->framebuffer = NULL;
		}
		else
		{
			// 非PAINT消息。先控件后视窗顺序
			// 如果存在子控件，将消息派发到子控件
			for (i = 0; i < pWnd->cbWidget; i ++)
			{
				// KEYDOWN, KEYUP, XM_CHAR等输入消息仅发给聚焦控件
				if( (node->lpWidgetFlag[i] & (WDGT_FOCUSED|WDGT_ENABLE|WDGT_VISUAL)) == (WDGT_FOCUSED|WDGT_ENABLE|WDGT_VISUAL))
				{
					XM_WidgetProc (pWnd->lpWidget + i, node->lpWidgetFlag[i], node->UserData[i], &msg);
				}
			}

			if(msg.message == XM_SYSTEMEVENT)
			{
				// 窗口缺省处理
				(*pWnd->lpfnWndProc) (&msg);
				// 检查系统事件消息是否需要继续传递到系统缺省处理
				if(msg.option & XMMSG_OPTION_SYSTEMEVENT_DEFAULT_PROCESS)
				{
					// 转发给缺省消息处理过程处理(桌面负责所有系统事件的缺省处理)
					XM_DefaultSystemEventProc (hWnd, &msg);
					// XMHWND *pDesktopWnd = XMHWND_HANDLE(Desktop);
					// (*pDesktopWnd->lpfnWndProc) (&msg);
				}
			}
			else
			{
				if(msg.message == XM_TOUCHDOWN || msg.message == XM_TOUCHMOVE)
				{
					unsigned int osd_layer;
					int x, y;
					x = LOWORD(msg.lp);
					y = HIWORD(msg.lp);
					// 将触摸屏坐标转换为OSD坐标
					if(pWnd->type == HWND_ALERT)
					{
						// OSD 2
						osd_layer = XM_OSD_LAYER_2;
					}
					else
					{
						// OSD 1
						osd_layer = XM_OSD_LAYER_1;
					}
					//if(XM_lcdc_lcd_coordinate_to_osd_coordinate (XM_LCDC_CHANNEL_0, osd_layer, &x, &y) == 0)
					{
						// 触摸点位于OSD区域内部
						msg.lp = MAKELONG(x,y);
						(*pWnd->lpfnWndProc) (&msg);
					}
				}
				else
				{
					(*pWnd->lpfnWndProc) (&msg);
				}
			}
		}
		node->flag = (BYTE)(node->flag & ~HWND_DISPATCH);
	}

	return dwExitCode;
}

xm_osd_framebuffer_t XM_GetWindowFrameBuffer (HANDLE hWnd)
{
	int i;

	for (i = 0; i < window_count; i++)
	{
		if(window_stack[i].hwnd == hWnd)
		{
			return window_stack[i].framebuffer;
		}
	}
	return NULL;
}

// 设置视窗关联的framebuffer对象
XMBOOL XM_SetWindowFrameBuffer (HANDLE hWnd, xm_osd_framebuffer_t framebuffer)
{
	// 检查窗口的实例化对象
	HWND_NODE *node = get_node_from_hwnd (hWnd);
	if(node == NULL)
		return 0;

	node->framebuffer = framebuffer;
	return 1;
}

// 设置视图切换时Animating效果方向
void XM_SetViewSwitchAnimatingDirection (UINT AnimatingDirection)
{
	view_animating_direction = AnimatingDirection;
}
