//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_touch_drv.c
//	  触摸事件驱动
//
//	Revision history
//
//		2010.09.02	ZhuoYongHong Initial version
//
//****************************************************************************

#include <xm_type.h>
#include <xm_user.h>
#include <xm_dev.h>
#include <xm_base.h>
#include <xm_key.h>
#include <xm_printf.h>

extern int backlight_clear_auto_off_ticket (void);


typedef struct {
	DWORD		point;
	DWORD		type;
} TOUCH_EVENT;

#define	TOUCH_BUFFER_SIZE	0x1F		/* maximum key strobe buffer */


static TOUCH_EVENT			TouchBuffer[TOUCH_BUFFER_SIZE+1];		// 循环队列
static volatile	BYTE	   TouchBPos;			// 循环队列首部
static volatile	BYTE		TouchEPos;			// 循环队列尾部
static volatile	BYTE		TouchInit = 0;		// 标识触摸驱动是否已初始化


// 投递硬件触摸事件, 由外部硬件驱动调用
// 加入到队尾
// 投递触摸事件到消息队列
// point		触摸位置
// type		触摸事件类型
// 返回值定义
//		1		事件投递到事件缓冲队列成功
//		0		事件投递到事件缓冲队列失败
unsigned char XM_TouchEventProc (unsigned long point, unsigned int type)
{
	TOUCH_EVENT *tp;

	XM_lock ();
	if(TouchInit == 0)
	{
		XM_unlock ();
		return 0;
	}

	// 检查自动测试是否存在。自动测试下禁止触摸操作
	//if(TesterSendCommand(TESTER_CMD_TEST, NULL))
	//	return 0;

	// 检查队列已满
	if( ((TouchEPos + 1) & TOUCH_BUFFER_SIZE) == (TouchBPos & TOUCH_BUFFER_SIZE) )
	{
		XM_unlock ();
		return 0;
	}

	tp = TouchBuffer + (TouchEPos & TOUCH_BUFFER_SIZE);
	tp->point	= point;
	tp->type = type;
	TouchEPos ++;

	XM_wakeup ();

	XM_unlock ();

	return 1;
}

XMBOOL XM_TouchDriverGetEvent (DWORD *point, DWORD *type)
{
      // int x, y;
	TOUCH_EVENT *tp;
	XMBOOL ret = FALSE;
	
	if(TouchInit == 0)
		return FALSE;
	
	XM_lock ();	// 保护互斥访问
	if( TouchBPos != TouchEPos )
	{
		ret = TRUE;
		tp = TouchBuffer + (TouchBPos & TOUCH_BUFFER_SIZE);


	     // x = LOWORD(tp->point)-104;
	      //  y = HIWORD(tp->point)-40;
		//tp->point=MAKELONG(x,y);
		*point = tp->point;

		*type = tp->type;

		TouchBPos ++;
				//XM_printf ("x  %d,y  %d\n", x, y);
	}
	XM_unlock ();
	
	if(ret)
	{
		// 返回值为1背光重新开启
		
		// 触摸事件，开启背光
		if(backlight_clear_auto_off_ticket ())
		{
			// 将该触摸事件过滤
			ret = 0;
		}
	}

	return ret;
}

// 触摸驱动打开
void XM_TouchDriverOpen (void)
{
	XM_lock ();
	
	TouchBPos = TouchEPos = 0;

	TouchInit = 1;
		
	XM_unlock ();
}

// 触摸驱动关闭
void XM_TouchDriverClose (void)
{
	XM_lock ();

	TouchInit = 0;

	TouchBPos = TouchEPos = 0;
	
	XM_unlock ();
}


//=========================
// 查询是否存在未读的触摸事件
XMBOOL XM_TouchDriverPoll(void)
{
	XMBOOL ret = FALSE;

	XM_lock();	// 保护互斥访问
	if( TouchEPos != TouchBPos )	/* Key Event buffer empty */
		ret = TRUE;
	XM_unlock();

	return ret;
}

//=====================