//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_kbd_drv.c
//	  键盘事件驱动
//
//	Revision history
//
//		2010.09.01	ZhuoYongHong Initial version
//
//****************************************************************************

#include <xm_type.h>
#include <xm_user.h>
#include <xm_dev.h>
#include <xm_base.h>
#include <xm_key.h>
#include <xm_printf.h>
#include "types.h"
typedef struct {
	WORD		key;
	WORD		mod;
} KEY_EVENT;

#define	KEY_BUFFER_SIZE	0x1F		/* maximum key strobe buffer */


static KEY_EVENT		    KeyBuffer[KEY_BUFFER_SIZE+1];		// 循环队列
static volatile	BYTE	    KeyBPos;			// 循环队列首部
static volatile	BYTE		KeyEPos;			// 循环队列尾部
static volatile	BYTE		keyInit = 0;		// 标识按键驱动是否已初始化


// 投递硬件按键事件, 由外部硬件驱动调用
// 加入到队尾
unsigned char XM_KeyEventProc (unsigned short key, unsigned short mod)
{
	KEY_EVENT *tp;

	XM_lock ();
	if(keyInit == 0)
	{
		XM_unlock ();
		return 0;
	}

 
	if(key == KEY_NULL)
	{
	    printf("null key \n"); 
		XM_unlock ();
		return 0;
	}
	// 检查自动测试是否存在。自动测试下禁止按键操作
	//if(TesterSendCommand(TESTER_CMD_TEST, NULL))
	//	return 0;

	// 检查队列已满
	if( ((KeyEPos + 1) & KEY_BUFFER_SIZE) == (KeyBPos & KEY_BUFFER_SIZE) )
	{
		XM_unlock ();
		return 0;
	}

	tp = KeyBuffer + (KeyEPos & KEY_BUFFER_SIZE);
	tp->key	= key;
	if(	key == VK_AP_SYSTEM_EVENT || key == VK_AP_VIDEOSTOP)
	{
		tp->mod	= (unsigned short)(mod);
	}
	else
	{
		tp->mod	= (unsigned short)(mod|XMKEY_STROKE);
	}
	KeyEPos ++;

	XM_wakeup ();

	XM_unlock ();

	return 1;
}
extern BOOL Close_Audio_Sound;
XMBOOL XM_KeyDriverGetEvent (WORD *key, WORD *modifier)
{
	KEY_EVENT *tp;
	XMBOOL ret = FALSE;
	
	if(keyInit == 0)
		return FALSE;
	
	XM_lock ();	// 保护互斥访问
	if( KeyBPos != KeyEPos )
	{
		ret = TRUE;
		tp = KeyBuffer + (KeyBPos & KEY_BUFFER_SIZE);
		*key = tp->key;
		*modifier = tp->mod;
		KeyBPos ++;
	}
	XM_unlock ();
	
    //背光关闭之后,需要打开背光
    if(ret && Close_Audio_Sound)
    {
        //判定当前背光是是开还是关
        if(XM_GetFmlDeviceCap(DEVCAP_OSD) == 0) {
            backlight_clear_auto_off_ticket();
            ret = 0; //该按键事件被清空
        }else { // 重新计时
            backlight_clear_auto_off_ticket();
        }
    }
    #if 0
	if(ret)
	{
		// 返回值为1背光重新开启
		int backlight_clear_auto_off_ticket (void);
		int card_event_to_enable_backlight = 0;
		unsigned int event = *modifier;
		
		if(*key == VK_AP_SYSTEM_EVENT && (event == SYSTEM_EVENT_CARD_INSERT || event == SYSTEM_EVENT_CARD_UNPLUG 
													 || event == SYSTEM_EVENT_CARD_FS_ERROR || event == SYSTEM_EVENT_CARD_INVALID))
			card_event_to_enable_backlight = 1;

		// 按键事件(+卡插拔事件)，开启背光
		if( (*modifier & XMKEY_STROKE) || card_event_to_enable_backlight)
		{
			if(backlight_clear_auto_off_ticket ())
			{
				if(*modifier & XMKEY_STROKE)		// 按键
				{
					// 将该键过滤
					ret = 0;
				}
			}
		}
	}
    #endif

	return ret;
}

// 按键驱动打开
void XM_KeyDriverOpen (void)
{
	XM_lock ();
	
	KeyBPos = KeyEPos = 0;

	keyInit = 1;
	
	// 向内核注册按键事件通知回调函数
	//XM_RegisterKeyEventProc (XM_KeyEventProc);
	
	XM_unlock ();
}

// 按键驱动关闭
void XM_KeyDriverClose (void)
{
	XM_lock ();
	// 向内核注销按键事件通知回调函数
	//XM_RegisterKeyEventProc (NULL);

	keyInit = 0;

	KeyBPos = KeyEPos = 0;
	
	XM_unlock ();
}


//=========================
// 查询是否存在未读的按键事件
XMBOOL XM_KeyDriverPoll(void)
{
	XMBOOL ret = FALSE;

	XM_lock();	// 保护互斥访问
	if( KeyEPos != KeyBPos )	/* Key Event buffer empty */
		ret = TRUE;
	XM_unlock();

	return ret;
}

//=====================

// 检查并投递硬件事件, 由外部硬件驱动调用
// 如果事件未在硬件消息队列中, 将该事件加入到队尾.
// 若该事件已在硬件消息队列中, 不做任何处理, 返回
// 返回值
//   1        表示该消息已投递到硬件队列
//   0        表示该消息未能投递到硬件队列, 消息投递失败
unsigned char XM_KeyEventPost (unsigned short key, unsigned short mod)
{
	KEY_EVENT *tp;
	unsigned char ret = 0;
	BYTE head, tail;
	
	XM_lock ();
	do 
	{
		if(keyInit == 0)
		{
			break;
		}
		
		// 遍历, 检查硬件事件是否已存在
		head = KeyBPos;
		tail = KeyEPos;
		while(head != tail)
		{
			tp = KeyBuffer + (head & KEY_BUFFER_SIZE);
			if(tp->key == key && tp->mod == mod)
			{
				ret = 1;
				break;
			}
			head ++;
		}
		
		if(ret == 1)
			break;
		
		// 事件未存在
		ret = XM_KeyEventProc (key, mod);		
		
	} while (0);
	XM_unlock ();
	
	return ret;
}
