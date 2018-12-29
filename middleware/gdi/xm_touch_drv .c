//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_touch_drv.c
//	  �����¼�����
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


static TOUCH_EVENT			TouchBuffer[TOUCH_BUFFER_SIZE+1];		// ѭ������
static volatile	BYTE	   TouchBPos;			// ѭ�������ײ�
static volatile	BYTE		TouchEPos;			// ѭ������β��
static volatile	BYTE		TouchInit = 0;		// ��ʶ���������Ƿ��ѳ�ʼ��


// Ͷ��Ӳ�������¼�, ���ⲿӲ����������
// ���뵽��β
// Ͷ�ݴ����¼�����Ϣ����
// point		����λ��
// type		�����¼�����
// ����ֵ����
//		1		�¼�Ͷ�ݵ��¼�������гɹ�
//		0		�¼�Ͷ�ݵ��¼��������ʧ��
unsigned char XM_TouchEventProc (unsigned long point, unsigned int type)
{
	TOUCH_EVENT *tp;

	XM_lock ();
	if(TouchInit == 0)
	{
		XM_unlock ();
		return 0;
	}

	// ����Զ������Ƿ���ڡ��Զ������½�ֹ��������
	//if(TesterSendCommand(TESTER_CMD_TEST, NULL))
	//	return 0;

	// ����������
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
	
	XM_lock ();	// �����������
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
		// ����ֵΪ1�������¿���
		
		// �����¼�����������
		if(backlight_clear_auto_off_ticket ())
		{
			// ���ô����¼�����
			ret = 0;
		}
	}

	return ret;
}

// ����������
void XM_TouchDriverOpen (void)
{
	XM_lock ();
	
	TouchBPos = TouchEPos = 0;

	TouchInit = 1;
		
	XM_unlock ();
}

// ���������ر�
void XM_TouchDriverClose (void)
{
	XM_lock ();

	TouchInit = 0;

	TouchBPos = TouchEPos = 0;
	
	XM_unlock ();
}


//=========================
// ��ѯ�Ƿ����δ���Ĵ����¼�
XMBOOL XM_TouchDriverPoll(void)
{
	XMBOOL ret = FALSE;

	XM_lock();	// �����������
	if( TouchEPos != TouchBPos )	/* Key Event buffer empty */
		ret = TRUE;
	XM_unlock();

	return ret;
}

//=====================