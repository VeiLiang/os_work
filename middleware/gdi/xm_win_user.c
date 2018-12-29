//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_win_user.c
//	  �ں���Ϣ������
//
//	Revision history
//
//		2010.09.01	ZhuoYongHong Initial version
//		2012.01.20	ZhuoYongHong ����Alphaͨ������
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
// ��Ϣ���ж���
typedef struct _kmsg
{
	struct _kmsg*	prev;
	struct _kmsg*	next;
	XMMSG				msg;
} kmsg_s, *kmsg_t;

// �����������׼��֮��Ĺ�ϵ�����Ӵ����뷽ʽ����������
static void XM_init_view_reference_coordinate (void);

extern  int ShowLogo (UINT32 LogoAddr,UINT32 LogoSize);
extern int PowerOnShowLogo;

static kmsg_s	kmsg_buff[XM_MAX_MSG];
static queue_s	kmsg_pool;
static queue_s	mcb_post_msg;


// �Ӵ�ջ
static HWND_NODE	window_stack[MAX_HWND_STACK];

// �Ӵ���Widget��־
static BYTE			widget_flag[MAX_STACK_WIDGET_COUNT];

// �ؼ�(Widget)���û�����
static VOID *		widget_userdata[MAX_STACK_WIDGET_COUNT];

// �Ӵ�ջ���Ӵ��ĸ���
static BYTE			window_count;

// �Ӵ�ջ��Widget�ĸ���
static BYTE			widget_count;

//	XM_JUMP_POPDEFAULT = 0,			//	���Ӵ�ջ�������Ӵ����(��ȥ����)��Ȼ�������µ��Ӵ�
//	XM_JUMP_POPDESKTOP,				// ���Ӵ�ջ�������Ӵ����(��������)��Ȼ�������µ��Ӵ�
//	XM_JUMP_POPTOPVIEW,				// ���Ӵ�ջ��ջ���Ӵ������Ȼ�������µ��Ӵ�
static XM_JUMP_TYPE		jump_type;		// ������תʱ�Ƿ񵯳������Ӵ�/���档

static HANDLE	jumpWnd;
static HANDLE	pushWnd;
static HANDLE	pullWnd;

static DWORD	customData;		// ���ڴ���ʱ�������ݣ����ض�������ء�
										// ͨ���������ݣ��ɶԴ�����Ϊ���ж���
										// ��������ͨ��XM_ENTER��Ϣ��lp��������
										// 0 ��ʾȱʡ����

// �Ӵ�����ʱanimatingЧ����ȱʡ����
static UINT		view_animating_direction = XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP;

// view����Ͷ�����
// ���������Ӵ��Ķ�������( _x, _y, _cx, _cy ) �� ��׼���Ļ�׼���� (0, 0, LCD_XDOTS, LCD_YDOTS) ֮��Ĺ�ϵ��
//		����ÿ���Ӵ�ӳ�䵽Ŀ��Ͷ��ϵͳ֮��ı��������뷽ʽ��
#define	XM_VIEW_ALIGN_HORZ_SCALE		0x00000001			// �Ӵ�ˮƽ��������
#define	XM_VIEW_ALIGN_HORZ_LEFT		0x00000002			// �Ӵ�ˮƽ�������(����ڻ�׼��)
#define	XM_VIEW_ALIGN_HORZ_RIGHT		0x00000004			// �Ӵ�ˮƽ���Ҷ���(����ڻ�׼��)
#define	XM_VIEW_ALIGN_HORZ_CENTRE	0x00000008			// �Ӵ�ˮƽ���ж���(����ڻ�׼��)
#define	XM_VIEW_ALIGN_VERT_SCALE		0x00000010			// �Ӵ���ֱ��������
#define	XM_VIEW_ALIGN_VERT_TOP		0x00000020			// �Ӵ���ֱ���϶���(����ڻ�׼��)
#define	XM_VIEW_ALIGN_VERT_BOTTOM	0x00000040			// �Ӵ���ֱ���¶���(����ڻ�׼��)
#define	XM_VIEW_ALIGN_VERT_CENTRE	0x00000080			// �Ӵ���ֱ���ж���(����ڻ�׼��)

static float	view_default_horz_ratio;		// ϵͳȱʡ�Ӵ�ˮƽ���ű�������(�����������׼��֮��Ĺ�ϵ����)
static float	view_default_vert_ratio;		//	ϵͳȱʡ�Ӵ���ֱ���ű�������(�����������׼��֮��Ĺ�ϵ����)
static unsigned int	view_default_align;		// ϵͳȱʡ���뷽ʽ(�����������׼��֮��Ĺ�ϵ����)

XMHWND_DECLARE(Desktop)
XMHWND_DECLARE(FactoryView)

extern void	winmem_save_environment (unsigned char *buff, unsigned int size);
extern void	winmem_restore_environment (unsigned char *buff, unsigned int size);

// ����XMϵͳ�����л���(�ڲ�����)
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

// �ָ�XMϵͳ�����л���(�ڲ�����)
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
	// ������ѹջ���Ӵ��Ŀ���λ����Ϣ
	for (i = 0; i < window_count; i ++)
	{
		HWND_NODE *node;
		node = &window_stack[i];
		if(node->hwnd)
		{
			XMHWND *pWnd;
			// ��ȡ�Ӵ��ṹ��ָ��
			pWnd = ADDRESS_OF_HANDLE (node->hwnd);

			// ���������Ӵ��Ķ�������( _x, _y, _cx, _cy ) �� ��׼���Ļ�׼���� (0, 0, LCD_XDOTS, LCD_YDOTS) ֮��Ĺ�ϵ��
			// ����ÿ���Ӵ�ӳ�䵽Ŀ��Ͷ��ϵͳ֮��ı��������뷽ʽ��

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


// ��Ϣϵͳ���Ӵ�ϵͳ��ʼ��
void winuser_init (void)
{
	BYTE i;

	// ��Ϣ���г�ʼ��
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

	// �����������׼��֮��Ĺ�ϵ�����Ӵ����뷽ʽ����������
	XM_init_view_reference_coordinate ();

}

// ��Ϣϵͳ���Ӵ�ϵͳ�ر�
void winuser_exit (void)
{
	XM_osd_framebuffer_exit ();
	//winuser_init ();
}

// ����Ϣ����Ͷ����Ϣ
XMBOOL	XM_PostMessage (WORD message, WORD wp, DWORD lp)
{
	kmsg_t	kmsg;
	XMMSG	*msg;

	// ����Ƿ����ڵȴ��Ӵ�PUSH��JUMP��PULL����
	if(jumpWnd || pushWnd || pullWnd)
	{
		// ����Ƿ���ϵͳ�¼���Ϣ, �������κ�ʱ��Ͷ��
		if(message == XM_SYSTEMEVENT)
		{
		}
		// �����û��Զ�����Ϣ�ɷ�������ͬ����֮�䴫��Ӧ���Զ�����Ϣ
		else if(message < XM_USER)
			return 0;
	}
	#if 0
	if(message == XM_KEYDOWN)
	{
		// ������
		XM_Beep (XM_BEEP_KEYBOARD);

	}
	#endif

	// ����Ƿ����Զ����ԡ��Զ������½�ֹTIMER��Ϣ
	//if(TesterSendCommand (TESTER_CMD_TEST, NULL))
	//{
	//	// Autotest running, disable TIMER to violate autotest process
	//	if(message == XM_TIMER)
	//	{
	//		return TRUE;
	//	}
	//}

	// ����ظ���XM_MCI��Ϣ
	if(message == XM_MCI)
	{
		// �����Ϣ�����Ƿ�Ϊ��
		if(!queue_empty (&mcb_post_msg))
		{
			// ���������е���Ϣ
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
		// �����Ϣ�����Ƿ�Ϊ��
		if(!queue_empty (&mcb_post_msg))
		{
			// ���������е���Ϣ
			kmsg = (kmsg_t)queue_next (&mcb_post_msg);
			while(kmsg != (kmsg_t)&mcb_post_msg)
			{
				msg = &kmsg->msg;
				if(msg->message == XM_TIMER && wp == msg->wp)
				{
					// �ҵ�ͬһ��ID��TIMER��Ϣ
					// ɾ������Ϣ��������Ϣ���뵽����β��
					queue_delete ((queue_s *)kmsg);
					// ����Ϣ��Ԫ�ͷ�
					del_kmsg (kmsg);
					break;
				}
				kmsg = (kmsg_t)queue_next ((queue_s *)kmsg);
			}
		}
	}
	// ����ظ���PAINT��Ϣ
	else if(message == XM_PAINT)
	{
		// �����Ϣ�����Ƿ�Ϊ��
		if(!queue_empty (&mcb_post_msg))
		{
			// ���������е���Ϣ
			kmsg = (kmsg_t)queue_next (&mcb_post_msg);
			while(kmsg != (kmsg_t)&mcb_post_msg)
			{
				msg = &kmsg->msg;
				if(msg->message == XM_PAINT)
				{
					// �ҵ�PAINT��Ϣ
					return 1;
				}
				kmsg = (kmsg_t)queue_next ((queue_s *)kmsg);
			}
		}
	}

	kmsg = new_kmsg (message, wp, lp);
	if(kmsg == NULL)
	{
		// ��Ϣ��������ǿ��ɾ������δ�����KeyDown��KeyUp��Ϣ
		// ���������е���Ϣ
		kmsg = (kmsg_t)queue_next (&mcb_post_msg);
		while(kmsg != (kmsg_t)&mcb_post_msg)
		{
			msg = &kmsg->msg;
			if(msg->message == XM_KEYDOWN || msg->message == XM_KEYUP)
			{
				// �ҵ�ͬһ��ID��TIMER��Ϣ
				// ɾ������Ϣ��������Ϣ���뵽����β��
				queue_delete ((queue_s *)kmsg);
				// ����Ϣ��Ԫ�ͷ�
				del_kmsg (kmsg);
				break;
			}
			kmsg = (kmsg_t)queue_next ((queue_s *)kmsg);
		}

		kmsg = new_kmsg (message, wp, lp);
		if(kmsg == NULL)
		{
			// ϵͳ���ش���
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


// ����Ϣ���ж��״�����ָ������Ϣ
XMBOOL	XM_InsertMessage (WORD message, WORD wp, DWORD lp)
{
	kmsg_t	kmsg;
	XMMSG		*msg;
	queue_s *head;

	// XM_printf ("XM_InsertMessage message=%d, wp=%d, lp=%d\n", message, wp, lp);

	// ����Ƿ����ڵȴ��Ӵ�PUSH��JUMP��PULL����
	if(jumpWnd || pushWnd || pullWnd)
		return 0;

	kmsg = new_kmsg (message, wp, lp);
	if(kmsg == NULL)
	{
		// ��Ϣ��������ǿ��ɾ������δ�����KeyDown��KeyUp��Ϣ
		// ���������е���Ϣ
		kmsg = (kmsg_t)queue_next (&mcb_post_msg);
		while(kmsg != (kmsg_t)&mcb_post_msg)
		{
			msg = &kmsg->msg;
			if(msg->message == XM_KEYDOWN || msg->message == XM_KEYUP)
			{
				// �ҵ�ͬһ��ID��TIMER��Ϣ
				// ɾ������Ϣ��������Ϣ���뵽����β��
				queue_delete ((queue_s *)kmsg);
				// ����Ϣ��Ԫ�ͷ�
				del_kmsg (kmsg);
				break;
			}
			kmsg = (kmsg_t)queue_next ((queue_s *)kmsg);
		}

		kmsg = new_kmsg (message, wp, lp);
		if(kmsg == NULL)
		{
			// ϵͳ���ش���
			return 0;
		}
	}

	if(queue_empty (&mcb_post_msg))
	{
		queue_insert ((queue_s *)kmsg, &mcb_post_msg);
	}
	else
	{
		// ���뵽������Ϣ��ǰ��
		head = queue_head (&mcb_post_msg);
		queue_insert ((queue_s *)kmsg, head);
	}

	return 1;
}

void XM_PollEvent (XMBOOL bBlockCaller)
{
	// �����жϽ�ֹģʽ
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
    //������ͷ�ĵ�ѹ
    //OPEN_Video_System();
    //�������� -������NVP6214 �ж�����ʽ֮���Զ���
    //HW_Auido_SetLevel(AP_GetAudo_PWM()); //�����������
    //APP_SetAudio_Sound(0); //
    //�ػ�״̬,���䵱ǰ״̬
    APP_SetPowerOn_Memory(Close_Brighness);//���õ�ǰ�����ģʽ
}
static VOID PollHardwareEvent (XMBOOL bBlockCaller)
{
	WORD key, mod;
	DWORD point, type;
	BYTE HasEvent = 0;

	// ��ѯ���ȴ�Ӳ���¼�
	//XM_PollEvent (bBlockCaller);

	// �����¼�����
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
				// �������ػ�
			   if(Close_Audio_Sound) { //��һ��
    			    //PowerOnShowLogo=0;//�����ػ�LOGO
    			    //�رձ���
    			    Close_Audio_Sound = FALSE;
                    HW_LCD_BackLightOff();
                    LED_Close_Brightness();//�ص�
                    //�ر�����ͷ�ĵ�ѹ
                    //Close_System();
                    //������
                    HW_Auido_SetLevel(0); //�����������
                    XM_voice_prompts_remove_all_voice();
                    //APP_SetAudio_Sound(0); //
                    //�ػ�״̬,���䵱ǰ״̬
                    APP_SetPowerOn_Memory(Close_Brighness);//���õ�ǰ�����ģʽ
                    APP_SaveMenuData ();
                    //������е�����
    			    //if(AP_GetLogo())
                    //    ShowLogo (ROM_T18_COMMON_DESKTOP_LOGO_POWEROFF_JPG,ROM_T18_COMMON_DESKTOP_LOGO_POWEROFF_JPG_SIZE);
    	            //OS_Delay (1000);
    				//XM_PostQuitMessage (0);
               }else { //��������
                    //���Զ��ػ�ģʽ�ر�
                    XM_PostQuitMessage (XMEXIT_REBOOT); //��������,��Ϊ���� power���ֲ���
               }
               #endif
			}
			// 20180129 zhuoyonghong
			// ���������������
			else if(key == VK_REBOOT)
			{
				XM_PostQuitMessage (XMEXIT_REBOOT);
			}
			// ��ϵͳ�¼�(���¼����ļ�ϵͳ�쳣������쳣��)��ģ�ⰴ����Ϣ����Ϊϵͳ�¼���Ϣ
			else if(key == VK_AP_SYSTEM_EVENT)
			{
			system_event_process:
				// ��ϵͳ�¼�������Ϣ����Ϊϵͳ�¼���Ϣ

				// �ж��Ƿ����ⲿ��Ƶ����豸(AVOUT��HDMI)���뵽ϵͳ�Ĵ����¼�
				if(	mod == SYSTEM_EVENT_AVOUT_PLUGOUT
					||	mod == SYSTEM_EVENT_AVOUT_PLUGIN
					|| mod == SYSTEM_EVENT_HDMI_PLUGOUT
					||	mod == SYSTEM_EVENT_HDMI_PLUGIN
					)
				{
					// AVOUT/HDMI �β��¼�, ִ�зֱ����л�����
					XM_PostQuitMessage (XMEXIT_CHANGE_RESOLUTION);
				}
				else
				{
					XM_PostMessage (XM_SYSTEMEVENT, mod, 0);
				}
			}
			else if(key == VK_AP_VIDEOSTOP)
			{
				// ����Ƶֹͣ������Ϣ����Ϊ��Ƶ��Ϣ
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

	// �����¼�����
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

	// ��ʱ���¼�����
	if(XM_TimerDriverPoll())
	{
		HasEvent = 1;
		XM_TimerDriverDispatchEvent ();	// ��鲢�ɷ���ʱ����Ϣ����Ϣ����
	}
	// �����������¼�
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
		// ��������͹���ģʽ
		XM_idle ();
	}

}

static XMBOOL _XM_PeekMessage(XMMSG *lpmsg,	BYTE bMsgFilterMin, BYTE bMsgFilterMax, XMBOOL bBlockCaller)
{
	XMMSG* msg;
	kmsg_t kmsg;
	XMBOOL rescan = 1;

	// �������е���Ϣ
	if(queue_empty (&mcb_post_msg))
	{
		// ��Ϣ����Ϊ��
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
			// �������κ���Ϣ
		}
		else
		{
			// ִ����Ϣ����
			if(msg->message >= bMsgFilterMin && msg->message <= bMsgFilterMax)
			{
				// �ҵ�ƥ����Ϣ
			}
			else
			{
				// ������һ����Ϣ
				kmsg = (kmsg_t) queue_next ((queue_s *)kmsg);
				continue;
			}
		}

		// ������Ϣ
		if(lpmsg)
		{
			lpmsg->wp = msg->wp;
			lpmsg->message = msg->message;
			lpmsg->lp = msg->lp;
			lpmsg->option = msg->option;
		}

		// ɾ�������е���Ϣ
		queue_delete ((queue_s *)kmsg);	// remove this kmsg item from kmsg queue
		del_kmsg (kmsg);

		return 1;
	}

	// ����ϢΪ�գ�ɨ��Ӳ���¼�()
	PollHardwareEvent (bBlockCaller);

	if(rescan)
	{
		rescan = 0;
		goto rescan_message;	// ɨ��MwSelect�����Ӳ��ʱ��
	}
	return 0;
}

// ����ָ������Ϣ��
// msg��ΪNULL����ɾ��ָ������Ϣ
// bMsgFilterMin = 0 && bMsgFilterMax = 0��ʾ����������Ϣ
// �ɹ�����TRUE��ʧ�ܷ���FALSE
XMBOOL 	XM_PeekMessage(XMMSG *msg,	BYTE bMsgFilterMin, BYTE bMsgFilterMax)
{
	return _XM_PeekMessage (msg, bMsgFilterMin, bMsgFilterMax, 0);
}

#if defined(PCVER)
static DWORD message_ticket = 0;

#if defined (__cplusplus)
	extern "C"{
#endif

DWORD xm_message_cycle = 25;		// 250���뷢��һ�β��԰���


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
		// PC���Խӿڣ���ȡ���԰���
		message_ticket ++;
		if( (message_ticket % xm_message_cycle) == 0)
		{
			XMTESTERMESSAGE Message;

			// ����Զ�������Ϣ
			if(XM_TesterSendCommand (TESTER_CMD_MESSAGE, &Message))
			{
				// ��õ���Ϣ���͸���ǰ����
				if(Message.wParam >= 0xF0 && Message.wParam <= 0xF2)	// ������/�γ�ģ�ⰴ��
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
						// ����
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
		// ���κ���Ϣ����
		// XM_Sleep (50);
	}

	// PC���Խӿڣ���¼�����¼�
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

// �ɷ���Ϣ
XMBOOL	XM_DispatchMessage (XMMSG *msg)
{
	HANDLE hWnd;
	XMHWND *pWnd;
	HWND_NODE *node;

	// �������Ӵ��л�����ʱ(PUSH��PULL��JUMP), ��ֹ�ⲿ��Ϣ�ɷ�
	if(pullWnd || pushWnd || jumpWnd)
		return 0;

	if(window_count == 0)
	{
#ifdef _WINDOWS
		XM_ASSERT (0);
#endif
		return 0;
	}

	// ����Ƿ���ڵݹ������Ϣ�ص�����
	node = &window_stack[window_count - 1];
	if(node->flag & HWND_DISPATCH)
	{
#ifdef _WINDOWS
		// ���ش��󡣲�����Ƕ�׵���
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

	// �����Ϣ�ɷ��У����ڼ���Ƿ���ڵݹ���Ϣ�ɷ�
	node->flag = (BYTE)(node->flag | HWND_DISPATCH);

	// ��ȡ�Ӵ��ṹ��ָ��
	pWnd = ADDRESS_OF_HANDLE (hWnd);
	XM_ASSERT (pWnd);

	// �����Ӵ��˳���Ϣ
	(*pWnd->lpfnWndProc) (msg);

	// ����ɷ��б�־
	node->flag = (BYTE)(node->flag & ~HWND_DISPATCH);

	return 1;
}

void XM_FlushMessage (void)
{
	kmsg_t kmsg, next;

	// �����Ϣ�����Ƿ�Ϊ��
	if(!queue_empty (&mcb_post_msg))
	{
		// ���������е���Ϣ
		kmsg = (kmsg_t)queue_next (&mcb_post_msg);
		while(kmsg != (kmsg_t)&mcb_post_msg)
		{
			next = (kmsg_t)queue_next ((queue_s *)kmsg);
			// ɾ������Ϣ��������Ϣ���뵽����β��
			queue_delete ((queue_s *)kmsg);
			// ����Ϣ��Ԫ�ͷ�
			del_kmsg (kmsg);
			kmsg = next;
		}
	}
}

// ɾ�����������з�COMMAND��Ϣ
void XM_FlushMessageExcludeCommandAndSystemEvent (void)
{
	kmsg_t kmsg, next;

	// �����Ϣ�����Ƿ�Ϊ��
	if(!queue_empty (&mcb_post_msg))
	{
		// ���������е���Ϣ
		kmsg = (kmsg_t)queue_next (&mcb_post_msg);
		while(kmsg != (kmsg_t)&mcb_post_msg)
		{
			next = (kmsg_t)queue_next ((queue_s *)kmsg);
			if(kmsg->msg.message != XM_COMMAND && kmsg->msg.message != XM_SYSTEMEVENT)
			{
				// ɾ������Ϣ��������Ϣ���뵽����β��
				queue_delete ((queue_s *)kmsg);
				// ����Ϣ��Ԫ�ͷ�
				del_kmsg (kmsg);
			}
			kmsg = next;
		}
	}
}

// ��ֹϵͳ�¼���Ϣ��ȱʡ����
VOID XM_BreakSystemEventDefaultProcess (XMMSG *msg)
{
	if(msg == NULL)
		return;
	if(msg->message != XM_SYSTEMEVENT)
		return;
	msg->option &= ~XMMSG_OPTION_SYSTEMEVENT_DEFAULT_PROCESS;
}


// ���ô��ڵ�˽������
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

// ��ȡ���ڵ�˽������
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


// ʹ����������Ч��ϵͳͶ��XM_PAINT��Ϣ����Ϣ����β��
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

// ϵͳ�����Ϣ�����е�XM_PAINT��Ϣ�������ڣ�����������Ϣ�����ײ����Ա�����һ����Ϣѭ����XM_PAINT��Ϣ������
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

// ��ѹ���µ��Ӵ�ʱ��ͬʱָ���´��ڵĶ�����Ϣ
// ������Ϣ(dwCustomData)ͨ��XM_ENTER��Ϣ��lp��������
XMBOOL	XM_PushWindowEx	(HANDLE hWnd, DWORD dwCustomData)
{
	XMHWND *pWnd;
	if(hWnd != XMHWND_HANDLE(Desktop) && XM_GetFmlDeviceCap(DEVCAP_SCREEN) == DEVCAP_SCREEN_DISCONNECT)
		return 0;

	// ��������ѹջ/���ڵ�ջ/������תǶ��
	if(jumpWnd || pushWnd || pullWnd)
	{
		XM_printf ("XM_PushWindowEx NG, jumpWnd or pushWnd or pullWnd is not empty\n");
		return 0;
	}

	if(!IsValidWindow(hWnd))
		return 0;

	// ����Ӵ�����
	if(window_count >= MAX_HWND_STACK)
	{
		return 0;
	}

	// ��ȡ�Ӵ��ṹ��ָ��
	pWnd = ADDRESS_OF_HANDLE (hWnd);

	if(pWnd->cbWidget)
	{
#ifdef _WINDOWS
		XM_ASSERT (pWnd->cbWidget <= MAX_STACK_WIDGET_COUNT);
#endif
		// �Ӵ��ж���Widget�ؼ�������Ƿ����㹻�Ŀؼ���־���ڷ���
		if( (widget_count + pWnd->cbWidget) > MAX_STACK_WIDGET_COUNT )
		{
			// ϵͳ���㹻��Դ�����µ��Ӵ�
			return 0;
		}
	}

	// ��¼ѹ���Ӵ�
	pushWnd = hWnd;
	customData = dwCustomData;

	return 1;
}

// XM_PushWindow
// �ڵ�ǰ�Ӵ���ѹ���µ��Ӵ���Ӧ������Ҫ����������ҿ����𼶷��ص����Ρ�
// ִ��Push��������Ϣ���б����
// ���治��Ҫѹ�뵽ջ�С���ջΪ��ʱ���Զ�������ѹ�뵽ջ����
// ���ֵ�Ӧ���У�������ʼ����-->�����������-->�ֵ����Ľ���-->�������Խ��棬���Ӵ�ջ��ʾ����
//    ջ��-->          �������Խ���
//                     �ֵ����Ľ���
//                     �����������
//    ջ��-->          ������ʼ����
XMBOOL	XM_PushWindow	(HANDLE hWnd)
{
	return XM_PushWindowEx (hWnd, HWND_CUSTOM_DEFAULT);
}

// XM_PullWindow
// 1) hWnd = 0, ����ջ���Ӵ�,���ڷ��ص�ǰһ���Ӵ���
//  ��� ���������Խ��桱 ���ص� ���ֵ����Ľ��桱
//
// 2) hWnd != 0, ѭ������ջ���Ӵ���ֱ���ҵ�ָ�����Ӵ�(hWnd)���������Ӵ���Ϊջ���Ӵ�
// �����һ���ַ������´� ���������Խ��桱 ���ص� ��������ʼ���桱
XMBOOL XM_PullWindow	(HANDLE hWnd)
{
	HWND_NODE *node;
	BYTE i;

	if(XM_GetFmlDeviceCap(DEVCAP_SCREEN) == DEVCAP_SCREEN_DISCONNECT)
		return 0;

	// ��������ѹջ/���ڵ�ջ/������תǶ��
	if(jumpWnd || pushWnd || pullWnd)
	{
		XM_printf ("XM_PullWindow NG, jumpWnd or pushWnd or pullWnd is not empty\n");
		return 0;
	}

	// ����Ӵ�ջ�Ƿ�Ϊ��
	if(window_count == 0)
	{
#ifdef _WINDOWS
		XM_ASSERT (0);
#endif
		return 0;
	}

	if(hWnd == 0)
	{
		// ����ջ��(��ǰ)����
		if(window_count == 1)
		{
			// ջ��ֻ��һ���Ӵ�
			// �л��������Ӵ���������ѹ��
			// pushWnd = XMHWND_HANDLE(Desktop);
			// ������ѹ��ʱ,ԭ���Ӵ�������ѹ��, ��û�е���. ��ΪJump��ʽ, ����ǰ�Ӵ����׵������ͷ���Դ
			jumpWnd = XMHWND_HANDLE(Desktop);
			return 1;
		}

		// ��¼�������Ӵ�
		pullWnd = window_stack[window_count-2].hwnd;
		return 1;
	}

	// ɨ����Ҫ�������Ӵ��Ƿ����Ӵ�ջ��
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

// ���Ӵ�ջ�������Ӵ������Ȼ�������µ��Ӵ���һ�����ڹ�������תʹ��
// ��������Ӵ�ջ����£����¡�ϵͳʱ�䡱���ܼ�����ֱ�ӵ���XM_WndGoto����ת����ϵͳʱ�䡱��
XMBOOL	XM_JumpWindow	(HANDLE hWnd)
{
	return XM_JumpWindowEx (hWnd, HWND_CUSTOM_DEFAULT, XM_JUMP_POPDESKTOP);
}

// XM_JumpWindow
// ���Ӵ�ջ��ջ���Ӵ��������Ӵ������Ȼ�������µ��Ӵ���һ�����ڹ�������תʹ��
// ��������Ӵ�ջ����£����¡�ϵͳʱ�䡱���ܼ�����ֱ�ӵ���XM_WndGoto����ת����ϵͳʱ�䡱��
//	XM_JUMP_POPDEFAULT = 0,			//	���Ӵ�ջ�������Ӵ����(��ȥ����)��Ȼ�������µ��Ӵ�
//	XM_JUMP_POPDESKTOP,				// ���Ӵ�ջ�������Ӵ����(��������)��Ȼ�������µ��Ӵ�
//	XM_JUMP_POPTOPVIEW,				// ���Ӵ�ջ��ջ���Ӵ������Ȼ�������µ��Ӵ�
XMBOOL	XM_JumpWindowEx	(HANDLE hWnd, DWORD dwCustomData, XM_JUMP_TYPE JumpType)
{
	XMHWND *pWnd;
	XMBOOL ret = 1;

	if(XM_GetFmlDeviceCap(DEVCAP_SCREEN) == DEVCAP_SCREEN_DISCONNECT)
		return 0;

	// ��������ѹջ/���ڵ�ջ/������תǶ��
	if(jumpWnd || pushWnd || pullWnd)
	{
		XM_printf ("XM_JumpWindowEx NG, jumpWnd or pushWnd or pullWnd is not empty\n");
		return 0;
	}

	// ��鴰�ھ��
	if(hWnd == NULL)
		return 0;
	if(!IsValidWindow (hWnd))
		return 0;

	jump_type = JumpType;

	// ��ȡ�Ӵ��ṹ��ָ��
	pWnd = ADDRESS_OF_HANDLE (hWnd);
	// ���Widget���
	if(pWnd->cbWidget > MAX_STACK_WIDGET_COUNT)
	{
		// ϵͳ����
		XM_ASSERT (0);

		ret = 0;
	}

	if(ret == 0)
		return 0;

	jumpWnd = hWnd;
	customData = dwCustomData;

	return 1;
}


// ������(�Ӵ���ؼ�)����ת��Ϊ��Ļ����
XMBOOL XM_ClientToScreen (HANDLE hWnd, XMPOINT *lpPoint)
{
	XMHWND *pWnd;

	if(hWnd == 0 || lpPoint == NULL)
		return 0;

	// ��ȡ���ڽṹ��ָ��
	pWnd = ADDRESS_OF_HANDLE (hWnd);

	lpPoint->x = (XMCOORD)(lpPoint->x + pWnd->view_x);
	lpPoint->y = (XMCOORD)(lpPoint->y + pWnd->view_y);

	return 1;
}

// ����Ļ����ת��Ϊ����(�Ӵ���ؼ�)����
XMBOOL XM_ScreenToClient (HANDLE hWnd, XMPOINT *lpPoint)
{
	XMHWND *pWnd;

	if(hWnd == 0 || lpPoint == NULL)
		return 0;

	// ��ȡ���ڽṹ��ָ��
	pWnd = ADDRESS_OF_HANDLE (hWnd);

	lpPoint->x = (XMCOORD)(lpPoint->x - pWnd->view_x);
	lpPoint->y = (XMCOORD)(lpPoint->y - pWnd->view_y);

	return 1;
}

// ��ȡ����(�Ӵ���ؼ�)��λ����Ϣ
XMBOOL XM_GetWindowRect (HANDLE hWnd, XMRECT* lpRect)
{
	XMHWND *pWnd;

	if(hWnd == 0 || lpRect == NULL)
		return 0;

	// ��ȡ���ڽṹ��ָ��
	pWnd = ADDRESS_OF_HANDLE (hWnd);
	lpRect->left = pWnd->view_x;
	lpRect->top = pWnd->view_y;
	lpRect->right = (XMCOORD)(lpRect->left + pWnd->view_cx - 1);
	lpRect->bottom = (XMCOORD)(lpRect->top + pWnd->view_cy - 1);
	return 1;
}

// ���ô���ʵ����λ��(�����ʾ��ԭ��)
XMBOOL XM_SetWindowPos (HANDLE hWnd,
								XMCOORD x, XMCOORD y,			// ��Ļ����
								XMCOORD cx, XMCOORD cy
								)
{
	XMRECT rectDesktop;
	XMHWND *pWnd = ADDRESS_OF_HANDLE(hWnd);

	if(hWnd == 0)
		return 0;

	// �������
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

	// ��ȡ���ڽṹ��ָ��
	pWnd->view_x = x;
	pWnd->view_y = y;
	pWnd->view_cx = cx;
	pWnd->view_cy = cy;

	XM_InvalidateWindow ();

	return 1;
}

// ��ȡ��ǰ�Ӵ�(��ջ���Ӵ�)��ΨһID
// ��ջ�д��ڶ��ͬһ�Ӵ�������Ӵ�ʱ��������ÿ���Ӵ���
BYTE	XM_GetWindowID	(VOID)
{
	if(window_count == 0)
	{
		return (BYTE)(-1);
	}

	return (BYTE)(window_count - 1);
}

// ȱʡ��Ϣ
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

// lprc������С(dx dy ��2������������unsigned char )
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

	// ���ؼ������Ƿ���Ч
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

	// ���ؼ������Ƿ���Ч
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

	// �۽�û���޸�
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

	// ���ؼ������Ƿ���Ч
	if(bWidgetIndex >= node->cbWidget)
		return 0;

	if(node->lpWidgetFlag[bWidgetIndex] & WDGT_SELECT)
	{
		if(bSelect == 0)		// δѡ��
		{
			// δѡ��
			node->lpWidgetFlag[bWidgetIndex] &= ~WDGT_SELECT;
			XM_InvalidateWidget (bWidgetIndex);
		}
	}
	else
	{
		if(bSelect)		// ��ѡ��
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

	// ���ؼ������Ƿ���Ч
	if(bWidgetIndex >= node->cbWidget)
		return 0;

	if(node->lpWidgetFlag[bWidgetIndex] & WDGT_ENABLE)
	{
		if(bEnable == 0)
		{
			// δʹ��
			node->lpWidgetFlag[bWidgetIndex] &= ~WDGT_ENABLE;
			XM_InvalidateWidget (bWidgetIndex);
		}
	}
	else
	{
		if(bEnable)		// ʹ��
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

	// ���ؼ������Ƿ���Ч
	if(bWidgetIndex >= node->cbWidget)
		return 0;

	if(node->lpWidgetFlag[bWidgetIndex] & WDGT_VISUAL)
	{
		if(bVisual == 0)		// ����
		{
			node->lpWidgetFlag[bWidgetIndex] &= ~WDGT_VISUAL;
			XM_InvalidateWindow ();
		}
	}
	else
	{
		if(bVisual)		// ��ʾ
		{
			node->lpWidgetFlag[bWidgetIndex] |= WDGT_VISUAL;
			XM_InvalidateWindow ();
		}
	}

	return 1;
}

// ��ȡ�ؼ����Ӵ��ӿؼ�����
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

// ��ȡ��ǰ�Ӵ��ľ��
HANDLE XM_GetWindow (VOID)
{
	HWND_NODE *node;
	node = get_node ();
	if(node == NULL)
		return NULL;

	return node->hwnd;
}

// ���ÿؼ����û�����
XMBOOL XM_SetWidgetUserData (BYTE bWidgetIndex, VOID *pUserData)
{
	HWND_NODE *node;

	node = get_node ();
	if(node == NULL)
		return 0;

	// ���ؼ������Ƿ���Ч
	if(bWidgetIndex >= node->cbWidget)
		return 0;

	node->UserData[bWidgetIndex] = pUserData;
	return 1;
}

// ��ȡ�ؼ����û�����
VOID *XM_GetWidgetUserData (BYTE bWidgetIndex)
{
	HWND_NODE *node;

	node = get_node ();
	if(node == NULL)
		return 0;

	// ���ؼ������Ƿ���Ч
	if(bWidgetIndex >= node->cbWidget)
		return 0;

	return node->UserData[bWidgetIndex];
}

// ��ȡ�Ӵ���ȫ��Alpha����
unsigned char XM_GetWindowAlpha (HANDLE hWnd)
{
	HWND_NODE *node = get_node_from_hwnd (hWnd);
	XMHWND *pWnd = (XMHWND *)hWnd;
	if(pWnd == NULL)
		return 0;
	node = get_node_from_hwnd (hWnd);
	// ��鴰���Ƿ�ʵ����
	if(node)
		return node->alpha;
	else
		return pWnd->alpha;
}

// �����Ӵ���ȫ��Alpha����
XMBOOL XM_SetWindowAlpha (HANDLE hWnd, unsigned char alpha)
{
	HWND_NODE *node;
	// ��ȡ���ڵ�ʵ����������
	node = get_node_from_hwnd (hWnd);
	if(node)
	{
		node->alpha = alpha;
		return 1;
	}
	else
		return 0;
}

// ��ָ����framebuffer���������hWnd, ���ڶ�������Ч�������
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

	// ������Ϣ�����д��ڵ�PAINT��Ϣ
	XM_PeekMessage (&msg, XM_PAINT, XM_PAINT);
	// �����Ӵ����־
	node->flag &= ~HWND_DIRTY;

	// �����Ϣ�ɷ��У����ڼ���Ƿ���ڵݹ���Ϣ�ɷ�
	//node->flag = HWND_DISPATCH;

	old_framebuffer = node->framebuffer;
	node->framebuffer = framebuffer;

	msg.message = XM_PAINT;
	msg.wp = 0;
	msg.lp = 0;

	// ��ȡ�Ӵ��ṹ��ָ��
	pWnd = ADDRESS_OF_HANDLE (hWnd);

	(*pWnd->lpfnWndProc) (&msg);

	// ���ӿؼ�����XM_PAINT��Ϣ
	for (i = 0; i < pWnd->cbWidget; i ++)
	{
		if(WIDGET_IS_VISUAL(node->lpWidgetFlag[i]))
			XM_WidgetProc (pWnd->lpWidget + i, node->lpWidgetFlag[i], node->UserData[i], &msg);
	}

	// node->framebuffer = NULL;
	node->framebuffer = old_framebuffer;

	//node->flag = (BYTE)(node->flag & ~HWND_DISPATCH);
}

// ʹ���Ӵ��������ر�ʱ�Ķ���Ч��
XMBOOL XM_EnableViewAnimate (HANDLE hWnd)
{
	HWND_NODE *node = get_node_from_hwnd (hWnd);
	if(node == NULL)
		return 0;
	node->flag |= HWND_ANIMATE;
	return 1;
}

// ��ֹ�Ӵ��������ر�ʱ�Ķ���Ч��
XMBOOL XM_DisableViewAnimate (HANDLE hWnd)
{
	HWND_NODE *node = get_node_from_hwnd (hWnd);
	if(node == NULL)
		return 0;
	node->flag &= ~HWND_ANIMATE;
	return 1;
}


// �����������׼��֮��Ĺ�ϵ�����Ӵ����뷽ʽ����������
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
		// �������
		if(pDesktopWnd->_cx == LCD_XDOTS)	// ˮƽ���У�ˮƽ���ű���Ϊ1.0
		{
			view_default_align = XM_VIEW_ALIGN_HORZ_CENTRE;
			view_default_horz_ratio = 1.0;
		}
		else
		{
			view_default_align = XM_VIEW_ALIGN_HORZ_LEFT;	// �����
			view_default_horz_ratio = (float)(((float)pDesktopWnd->_cx) / LCD_XDOTS);
		}
	}
	else if( (pDesktopWnd->_x + pDesktopWnd->_cx) == LCD_XDOTS )
	{
		// �Ҳ�����, �Ҷ���
		view_default_align = XM_VIEW_ALIGN_HORZ_RIGHT;
		view_default_horz_ratio = (float)(((float)pDesktopWnd->_cx + pDesktopWnd->_x) / LCD_XDOTS);
	}
	else
	{
		// ��������
		view_default_align = XM_VIEW_ALIGN_HORZ_SCALE;
		view_default_horz_ratio = (float)(((float)pDesktopWnd->_cx + pDesktopWnd->_x) / LCD_XDOTS);
	}

	if(pDesktopWnd->_y == 0)
	{
		// ��������
		if(pDesktopWnd->_cy == LCD_YDOTS)	// ��ֱ���У���ֱ���ű���Ϊ1.0
		{
			view_default_align |= XM_VIEW_ALIGN_VERT_CENTRE;
			view_default_vert_ratio = 1.0;
		}
		else
		{
			view_default_align |= XM_VIEW_ALIGN_VERT_TOP;	// ���϶���
			view_default_vert_ratio = (float)(((float)pDesktopWnd->_cy) / LCD_YDOTS);
		}
	}
	else if( (pDesktopWnd->_y + pDesktopWnd->_cy) == LCD_YDOTS )
	{
		// �ײ�����, ���¶���
		view_default_align |= XM_VIEW_ALIGN_VERT_BOTTOM;
		view_default_vert_ratio = (float)(((float)pDesktopWnd->_cy + pDesktopWnd->_y) / LCD_YDOTS);
	}
	else
	{
		// ��������
		view_default_align |= XM_VIEW_ALIGN_VERT_SCALE;
		view_default_vert_ratio = (float)(((float)pDesktopWnd->_cy + pDesktopWnd->_y) / LCD_YDOTS);
	}

	// ����Ŀ��ϵͳ���׼��֮��ı���
	osd_width = XM_lcdc_osd_get_width (0, XM_OSD_LAYER_1);
	osd_height = XM_lcdc_osd_get_height (0, XM_OSD_LAYER_1);


	view_default_horz_ratio = view_default_horz_ratio * osd_width / LCD_XDOTS;
	view_default_vert_ratio = view_default_vert_ratio * osd_height / LCD_YDOTS;

	// ����������ͼ
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
	BYTE bViewCloseAnimating;		// �ر��Ӵ�ʱ������Ч��
	const XMWDGT *widget;
	DWORD dwExitCode = 0;

	XMBOOL IsMessageLoop = TRUE;

	XMHWND *pDesktopWnd = XMHWND_HANDLE(Desktop);
	HANDLE hTopWnd = NULL;
	UINT uTopWndAnimateDirection = XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP;

	// ϵͳ����Ϣѭ��
	while (IsMessageLoop)
	{
		hWnd = 0;
		bViewCloseAnimating = 1;

		// ����Ƿ����Jump�Ӵ�
		if(jumpWnd)
		{
			// ��鶥�㴰�ڣ��Ƿ���ALERT���͵��Ӵ���
			// 1) ����ALERT�Ӵ��������ALERT�Ӵ��Ĺرչ���, �ָ�ALERT�Ӵ�����ǰ������OSD��Ŀ��Ʋ���
			// 2) �������������Ӵ�, ��ֱ�ӹرա�(��Ϊ�������͵��Ӵ����������в����ڶ�����OSD��������޸�)
			node = &window_stack[window_count - 1];
			if(node->flag & HWND_ANIMATE)
			{
				HANDLE hwnd = node->hwnd;
				if(((XMHWND *)ADDRESS_OF_HANDLE(hwnd))->type == HWND_ALERT)
				{
					// Alertǿ��Ϊ�Զ�����
					XM_osd_layer_animate (
						XM_OSDLAYER_ANIMATE_MODE_ALERT_REMOVE,
						XM_OSDLAYER_MOVING_FROM_TOP_TO_BOTTOM,
						hwnd,
						NULL
						);
				}
			}

			// ���Ӵ�ջ�е������Ӵ���ջ����ջ����ʼ����ֱ��ջ��
			node = &window_stack[window_count - 1];
			msg.message = XM_LEAVE;
			msg.wp = 0;
			msg.lp = 0;
			while(window_count)
			{
				hWnd =  node->hwnd;

				// ����Ƿ񵯳����洰��
				if(hWnd == XMHWND_HANDLE(Desktop))
				{
					if(jump_type == XM_JUMP_POPDEFAULT)
					{
						// ���������棬�˳�
						break;
					}
				}

				// �����Ϣ�ɷ��У����ڼ���Ƿ���ڵݹ���Ϣ�ɷ�
				node->flag = (BYTE)(node->flag | HWND_DISPATCH);

				// ��ȡ�Ӵ��ṹ��ָ��
				pWnd = ADDRESS_OF_HANDLE (hWnd);

				// ����ӿؼ��Ƿ���ڡ������ڣ����ӿؼ������˳���Ϣ
				for (i = 0; i < pWnd->cbWidget; i ++)
				{
					XM_WidgetProc (pWnd->lpWidget + i, node->lpWidgetFlag[i], node->UserData[i], &msg);
				}

				// �����Ӵ������Ӵ��˳���Ϣ
				(*pWnd->lpfnWndProc) (&msg);

				XM_ASSERT (pWnd->cbWidget <= widget_count);
				// �ͷ��Ӵ������Դ
				widget_count = (BYTE)(widget_count - pWnd->cbWidget);

				node->hwnd = 0;
				node->lpWidgetFlag = NULL;
				node->flag = 0;
				node->cbWidget = 0;
				node->UserData = 0;		// ���û�������Ч

				node --;

				window_count --;

				// ������ջ���Ӵ�
				if(jump_type == XM_JUMP_POPTOPVIEW)
					break;

			}

			jump_type = XM_JUMP_POPDEFAULT;

			// ����Ƿ���ת������(Desktop)
			if(jumpWnd == XMHWND_HANDLE(Desktop))
			{
				// ������洰���Ƿ��Ѵ���(����)
				if(window_count == 1 && window_stack[0].hwnd == jumpWnd)
				{
					XM_ASSERT (jumpWnd == node->hwnd);
					hWnd =  jumpWnd;
					jumpWnd = NULL;
					goto desktop_reenter;
				}
			}

			// ��¼��Ҫ�½��Ӵ�
			hWnd = jumpWnd;

			// ���JUMP�Ӵ�
			jumpWnd = 0;
		} // if(jumpWnd)
		// ����Ƿ����PUSH�Ӵ�
		else if(pushWnd)
		{
			// ����PUSH�Ӵ�ʱ���Ӵ�ջΪ��
			if(window_count > 0)
			{
				// ��ջ���Ӵ�(��ǰ�Ӵ�)����XM_LEAVE��Ϣ(��ʱ�뿪��Ϣ)
				msg.message = XM_LEAVE;
				msg.wp = 1;	// ��ʱ�뿪
				msg.lp = 0;

				node = &window_stack[window_count - 1];
				hWnd = node->hwnd;

				// ��鴰���Ƿ����롣���ǣ�����������
				if(pushWnd == hWnd)
				{
					XM_printf ("ERROR! Can't re-enter the same view (hwnd = 0x%08x)\n", hWnd);
					pushWnd = NULL;
					continue;
				}

				// �����Ϣ�ɷ��У����ڼ���Ƿ���ڵݹ���Ϣ�ɷ�
				node->flag = (BYTE)(node->flag | HWND_DISPATCH);

				// ��ȡ�Ӵ��ṹ��ָ��
				pWnd = ADDRESS_OF_HANDLE (hWnd);
				// ����ӿؼ��Ƿ���ڡ������ڣ����ӿؼ�����XM_LEAVE��Ϣ(��ʱ�뿪��Ϣ)
				for (i = 0; i < pWnd->cbWidget; i ++)
				{
					XM_WidgetProc (pWnd->lpWidget + i, node->lpWidgetFlag[i], node->UserData[i], &msg);
				}

				// ���Ӵ�����XM_LEAVE��Ϣ(��ʱ�뿪��Ϣ)
				(*pWnd->lpfnWndProc) (&msg);

				node->flag = (BYTE)(node->flag & ~HWND_DISPATCH);
			}

			// ��¼��Ҫ�½��Ӵ�
			hWnd = pushWnd;

			// ���PUSH�Ӵ�
			pushWnd = 0;
		}
		// ����Ƿ���ڵ����Ӵ�
		else if(pullWnd)
		{
			// ���ڵ����Ӵ�

			// ��ջ����ʼ����ֱ������ƥ����Ӵ����������Ӵ��˳���Ϣ
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

				// �����Ϣ�ɷ��У����ڼ���Ƿ���ڵݹ���Ϣ�ɷ�
				node->flag = (BYTE)(node->flag | HWND_DISPATCH);

				// ��ȡ�Ӵ��ṹ��ָ��
				pWnd = ADDRESS_OF_HANDLE (hWnd);
				// ����ӿؼ��Ƿ���ڡ������ڣ����ӿؼ������˳���Ϣ
				for (i = 0; i < pWnd->cbWidget; i ++)
				{
					XM_WidgetProc (pWnd->lpWidget + i, node->lpWidgetFlag[i], node->UserData[i], &msg);
				}

				// �����Ӵ��˳���Ϣ
				(*pWnd->lpfnWndProc) (&msg);

				XM_ASSERT (pWnd->cbWidget <= widget_count);
				// �ͷ��Ӵ������Դ
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
			// �����Ӵ����½�����Ϣ
			XM_ASSERT (window_count > 0);

			// ��ջ���Ӵ�(��ǰ�Ӵ�)����XM_ENTER��Ϣ(���½�����Ϣ)
			msg.message = XM_ENTER;
			msg.wp = 1;	// ���½�����Ϣ
			msg.lp = 0;

			// �����Ϣ�ɷ��У����ڼ���Ƿ���ڵݹ���Ϣ�ɷ�
			node->flag = (BYTE)(node->flag | HWND_DISPATCH);

			// ��ȡ�Ӵ��ṹ��ָ��
			pWnd = ADDRESS_OF_HANDLE (hWnd);
			// ���ӿؼ�����XM_ENTER��Ϣ
			for (i = 0; i < pWnd->cbWidget; i ++)
			{
				XM_WidgetProc (pWnd->lpWidget + i, node->lpWidgetFlag[i], node->UserData[i], &msg);
			}

			// �򴰿ڷ���XM_ENTER��Ϣ
			(*pWnd->lpfnWndProc) (&msg);


			if(	bViewCloseAnimating && node->flag & HWND_ANIMATE
				|| (hTopWnd && ((XMHWND *)ADDRESS_OF_HANDLE(hTopWnd))->type == HWND_ALERT)		// �����Ӵ�Ϊalertʱǿ�ƿ�������
				)
			{
				if(	(hTopWnd && ((XMHWND *)ADDRESS_OF_HANDLE(hTopWnd))->type == HWND_ALERT)
					)
				{
					// Alertǿ��Ϊ�Զ�����
					XM_osd_layer_animate (
						XM_OSDLAYER_ANIMATE_MODE_ALERT_REMOVE,
						XM_OSDLAYER_MOVING_FROM_TOP_TO_BOTTOM,
						hTopWnd,
						hWnd
						);
				}
				else if(hTopWnd && ((XMHWND *)ADDRESS_OF_HANDLE(hTopWnd))->type == HWND_EVENT)
				{
					// Eventǿ��Ϊ�Ե׶����Ƴ�
					XM_osd_layer_animate (
						XM_OSDLAYER_ANIMATE_MODE_EVENT_REMOVE,
						XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP,
						hTopWnd,
						hWnd
						);
				}
				else if(hWnd && hWnd == XMHWND_HANDLE(Desktop))		// ��������Ϊ����
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
					// λ��OSD 2�㣬��Ҫ�ֹ��رո�OSD��
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
					// ����XM_PAINT��Ϣ
					msg.message = XM_PAINT;
					msg.wp = 0;	// ���½�����Ϣ
					msg.lp = 0;

					// �����Ӵ����־
					node->flag &= ~HWND_DIRTY;
					// �򴰿ڷ���XM_PAINT��Ϣ
					(*pWnd->lpfnWndProc) (&msg);

					// ���ӿؼ�����XM_PAINT��Ϣ
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

			// ���PULL�Ӵ�
			pullWnd = 0;

			// ��������Ӵ���Ҫ����
			hWnd = 0;
		}

		// ����Ƿ�����½��Ӵ�
		if(hWnd)
		{
			node = &window_stack[window_count];

			// ��ȡ�Ӵ��ṹ��ָ��
			pWnd = ADDRESS_OF_HANDLE (hWnd);

			// �����ڳ�ʼ���ṹ�����ꡢ�ߴ���Ϣ��alpha���Ӹ��Ƶ��Ӵ��ڵ���ȥ����������޸�
			// ���������Ӵ��Ķ�������( _x, _y, _cx, _cy ) �� ��׼���Ļ�׼���� (0, 0, LCD_XDOTS, LCD_YDOTS) ֮��Ĺ�ϵ��
			// ����ÿ���Ӵ�ӳ�䵽Ŀ��Ͷ��ϵͳ֮��ı��������뷽ʽ��
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
				// �Ӵ��ж���Widget�ؼ�

				// ����ؼ���־
				node->lpWidgetFlag = widget_flag + widget_count;
				// ���Ƴ�ʼ��־
				widget = pWnd->lpWidget;

				flag = node->lpWidgetFlag;
				for (i = 0; i < pWnd->cbWidget; i ++)
				{
					*flag = widget->bFlag;
					flag ++;
					widget ++;
				}

				// ���ÿؼ��û�����, ȫ����ʼΪ�ա�
				// �Ӵ����ڽ���ʱΪ�����ӿؼ������û����ݣ��Ա��ӿؼ�����ʱ����ʹ���û�����
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
			node->flag = HWND_ANIMATE;		// ȱʡ֧�ֶ���Ч��
			node->PrivateData = NULL;

			window_count ++;

			// �����Ϣ������������Ϣ
			// XM_FlushMessage ();
			XM_FlushMessageExcludeCommandAndSystemEvent ();


			// �����Ӵ�����(XM_ENTER)��Ϣ
			msg.message = XM_ENTER;
			msg.wp = 0;
			msg.lp = customData;
			// �Զ����������Իָ�Ϊȱʡ����
			customData = HWND_CUSTOM_DEFAULT;

			// �����Ϣ�ɷ��У����ڼ���Ƿ���ڵݹ���Ϣ�ɷ�
			node->flag |= HWND_DISPATCH;

			(*pWnd->lpfnWndProc) (&msg);

			// ��鴴�������Ƿ���ֹ������ת����������
			if(jumpWnd || pushWnd || pullWnd)
			{
				node->flag &= ~HWND_DISPATCH;

				// �����Ӵ�����ʱ��Animating���򡣴��ڴݻ�ʱ�������ƶ�
				node->animatingDirection = view_animating_direction;

				// �ָ�ϵͳȱʡֵ
				view_animating_direction = XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP;
				continue;
			}

			// ����ӿؼ��Ƿ���ڡ������ڣ����ӿؼ����ͽ���(XM_ENTER)��Ϣ
			for (i = 0; i < pWnd->cbWidget; i ++)
			{
				XM_WidgetProc (pWnd->lpWidget + i, node->lpWidgetFlag[i], node->UserData[i], &msg);
			}

			// �����Ӵ�����ʱ��Animating���򡣴��ڴݻ�ʱ�������ƶ�
			node->animatingDirection = view_animating_direction;

			// �ָ�ϵͳȱʡֵ
			view_animating_direction = XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP;

			// ************ �������ڷ���XM_PAINT��Ϣ ***************
			if(node->flag & HWND_ANIMATE)
			{
				// ֧�ֶ���Ч��
				if(pWnd->type == HWND_VIEW)
				{
					// ��ͨ�Ӵ�
					XM_osd_layer_animate (
						XM_OSDLAYER_ANIMATE_MODE_VIEW_CREATE,
						node->animatingDirection,
						NULL,
						hWnd);
				}
				else if(pWnd->type == HWND_EVENT)
				{
					// ֪ͨ�Ӵ���ǿ���Զ����»���
					XM_osd_layer_animate (
						XM_OSDLAYER_ANIMATE_MODE_EVENT_CREATE,
						XM_OSDLAYER_MOVING_FROM_TOP_TO_BOTTOM,
						NULL,
						hWnd);
				}
				else
				{
					// Alert, ǿ�����¶����ƶ�
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
				// �Ƕ���Ч��
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

					// ����ӿؼ��Ƿ���ڡ������ڣ����ӿؼ�����ˢ����Ϣ
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

			// ����Ƿ�������Ӵ�����/�Ӵ���������
			if(jumpWnd || pushWnd || pullWnd)
				continue;
		}
		else
		{
			// ����Ӵ�ջ�Ƿ�Ϊ��
			if(window_count == 0)
			{
				XM_ASSERT (widget_count == 0);
				XM_PushWindow (XMHWND_HANDLE(Desktop));
				continue;
			}
		}

		// ��ȡ��һ����Ϣ
		if(!XM_GetMessage (&msg))
		{
			// ��Ϣѭ���˳���Ϣ

			// ����Ƿ��Ƿֱ����л����µ���Ϣѭ����ֹ
			if(msg.lp == XMEXIT_CHANGE_RESOLUTION)
			{
				if(XM_GetFmlDeviceCap(DEVCAP_SCREEN) == DEVCAP_SCREEN_DISCONNECT)
				{
					// ����ʾ�豸����
					// �ر�����(��ȥ����)��ǰ��ʾ���Ӵ�
					while(window_count > 0)
					{
						node = &window_stack[window_count - 1];
						hWnd = node->hwnd;
						// ��ȡ�Ӵ��ṹ��ָ��
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
							// �����Ϣ�ɷ��У����ڼ���Ƿ���ڵݹ���Ϣ�ɷ�
							node->flag = (BYTE)(node->flag | HWND_DISPATCH);
							// ����ӿؼ��Ƿ���ڡ������ڣ����ӿؼ������˳���Ϣ
							for (i = 0; i < pWnd->cbWidget; i ++)
							{
								XM_WidgetProc (pWnd->lpWidget + i, node->lpWidgetFlag[i], node->UserData[i], &leave_msg);
							}
							// �����Ӵ��˳���Ϣ
							(*pWnd->lpfnWndProc) (&leave_msg);

							XM_ASSERT (pWnd->cbWidget <= widget_count);

							// �ͷ��Ӵ������Դ
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
						// ��鶥���Ӵ�(TOP VIEW)�Ƿ��� HWND_EVENT ���� HWND_ALERT ����
						// ���ǣ����ö����Ӵ�(TOP VIEW)�ر�, (���� XM_LEAVE ��Ϣ)
						node = &window_stack[window_count - 1];
						hWnd = node->hwnd;
						// ��ȡ�Ӵ��ṹ��ָ��
						pWnd = ADDRESS_OF_HANDLE (hWnd);
						if( /* pWnd->type == HWND_EVENT ||*/ pWnd->type == HWND_ALERT )
						{
							XMMSG leave_msg;
							leave_msg.message = XM_LEAVE;
							leave_msg.wp = 0;
							leave_msg.lp = 0;
							// �����Ϣ�ɷ��У����ڼ���Ƿ���ڵݹ���Ϣ�ɷ�
							node->flag = (BYTE)(node->flag | HWND_DISPATCH);
							// ����ӿؼ��Ƿ���ڡ������ڣ����ӿؼ������˳���Ϣ
							for (i = 0; i < pWnd->cbWidget; i ++)
							{
								XM_WidgetProc (pWnd->lpWidget + i, node->lpWidgetFlag[i], node->UserData[i], &leave_msg);
							}
							// �����Ӵ��˳���Ϣ
							(*pWnd->lpfnWndProc) (&leave_msg);

							XM_ASSERT (pWnd->cbWidget <= widget_count);

							// �ͷ��Ӵ������Դ
							widget_count = (BYTE)(widget_count - pWnd->cbWidget);
							node->hwnd = 0;
							node->lpWidgetFlag = NULL;
							node->flag = 0;
							node->cbWidget = 0;
							node->UserData = 0;

							node --;

							window_count --;

							// OSD 2��ر�
							XM_osd_framebuffer_release (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_2);
							// ������ͬ����IO�Ĵ������ȴ�Ӳ��ͬ�����
							HW_lcdc_set_osd_coef_syn (XM_LCDC_CHANNEL_0, (1 << XM_OSD_LAYER_2));
						}
					}
				}
			}

			dwExitCode = msg.lp;
			break;
		}

		// �ɷ���Ϣ
		// ��ȡջ���Ӵ����
		node = &window_stack[window_count - 1];
		hWnd = node->hwnd;

		// �����Ϣ�ɷ��У����ڼ���Ƿ���ڵݹ���Ϣ�ɷ�
		node->flag = (BYTE)(node->flag | HWND_DISPATCH);

		// ��ȡ�Ӵ��ṹ��ָ��
		pWnd = ADDRESS_OF_HANDLE (hWnd);
		if(msg.message == XM_PAINT && pWnd != pDesktopWnd)
		{
			// PAINT��Ϣ���ȷ��͸��Ӵ����ٷ��͸��ؼ�
			XMBOOL PaintAll = 0;
			if(	pWnd->type == HWND_VIEW
				||	pWnd->type == HWND_EVENT
				)
			{
				// ��ͨ�Ӵ�
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
				// ALERT�Ӵ�
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

			// ��������ӿؼ�������Ϣ�ɷ����ӿؼ�
			for (i = 0; i < pWnd->cbWidget; i ++)
			{
				// ���ؼ��Ƿ�����Ҵ���������
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
			// ��PAINT��Ϣ���ȿؼ����Ӵ�˳��
			// ��������ӿؼ�������Ϣ�ɷ����ӿؼ�
			for (i = 0; i < pWnd->cbWidget; i ++)
			{
				// KEYDOWN, KEYUP, XM_CHAR��������Ϣ�������۽��ؼ�
				if( (node->lpWidgetFlag[i] & (WDGT_FOCUSED|WDGT_ENABLE|WDGT_VISUAL)) == (WDGT_FOCUSED|WDGT_ENABLE|WDGT_VISUAL))
				{
					XM_WidgetProc (pWnd->lpWidget + i, node->lpWidgetFlag[i], node->UserData[i], &msg);
				}
			}

			if(msg.message == XM_SYSTEMEVENT)
			{
				// ����ȱʡ����
				(*pWnd->lpfnWndProc) (&msg);
				// ���ϵͳ�¼���Ϣ�Ƿ���Ҫ�������ݵ�ϵͳȱʡ����
				if(msg.option & XMMSG_OPTION_SYSTEMEVENT_DEFAULT_PROCESS)
				{
					// ת����ȱʡ��Ϣ������̴���(���渺������ϵͳ�¼���ȱʡ����)
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
					// ������������ת��ΪOSD����
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
						// ������λ��OSD�����ڲ�
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

// �����Ӵ�������framebuffer����
XMBOOL XM_SetWindowFrameBuffer (HANDLE hWnd, xm_osd_framebuffer_t framebuffer)
{
	// ��鴰�ڵ�ʵ��������
	HWND_NODE *node = get_node_from_hwnd (hWnd);
	if(node == NULL)
		return 0;

	node->framebuffer = framebuffer;
	return 1;
}

// ������ͼ�л�ʱAnimatingЧ������
void XM_SetViewSwitchAnimatingDirection (UINT AnimatingDirection)
{
	view_animating_direction = AnimatingDirection;
}
