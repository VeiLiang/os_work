//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_kbd_drv.c
//	  �����¼�����
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


static KEY_EVENT		    KeyBuffer[KEY_BUFFER_SIZE+1];		// ѭ������
static volatile	BYTE	    KeyBPos;			// ѭ�������ײ�
static volatile	BYTE		KeyEPos;			// ѭ������β��
static volatile	BYTE		keyInit = 0;		// ��ʶ���������Ƿ��ѳ�ʼ��


// Ͷ��Ӳ�������¼�, ���ⲿӲ����������
// ���뵽��β
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
	// ����Զ������Ƿ���ڡ��Զ������½�ֹ��������
	//if(TesterSendCommand(TESTER_CMD_TEST, NULL))
	//	return 0;

	// ����������
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
	
	XM_lock ();	// �����������
	if( KeyBPos != KeyEPos )
	{
		ret = TRUE;
		tp = KeyBuffer + (KeyBPos & KEY_BUFFER_SIZE);
		*key = tp->key;
		*modifier = tp->mod;
		KeyBPos ++;
	}
	XM_unlock ();
	
    //����ر�֮��,��Ҫ�򿪱���
    if(ret && Close_Audio_Sound)
    {
        //�ж���ǰ�������ǿ����ǹ�
        if(XM_GetFmlDeviceCap(DEVCAP_OSD) == 0) {
            backlight_clear_auto_off_ticket();
            ret = 0; //�ð����¼������
        }else { // ���¼�ʱ
            backlight_clear_auto_off_ticket();
        }
    }
    #if 0
	if(ret)
	{
		// ����ֵΪ1�������¿���
		int backlight_clear_auto_off_ticket (void);
		int card_event_to_enable_backlight = 0;
		unsigned int event = *modifier;
		
		if(*key == VK_AP_SYSTEM_EVENT && (event == SYSTEM_EVENT_CARD_INSERT || event == SYSTEM_EVENT_CARD_UNPLUG 
													 || event == SYSTEM_EVENT_CARD_FS_ERROR || event == SYSTEM_EVENT_CARD_INVALID))
			card_event_to_enable_backlight = 1;

		// �����¼�(+������¼�)����������
		if( (*modifier & XMKEY_STROKE) || card_event_to_enable_backlight)
		{
			if(backlight_clear_auto_off_ticket ())
			{
				if(*modifier & XMKEY_STROKE)		// ����
				{
					// ���ü�����
					ret = 0;
				}
			}
		}
	}
    #endif

	return ret;
}

// ����������
void XM_KeyDriverOpen (void)
{
	XM_lock ();
	
	KeyBPos = KeyEPos = 0;

	keyInit = 1;
	
	// ���ں�ע�ᰴ���¼�֪ͨ�ص�����
	//XM_RegisterKeyEventProc (XM_KeyEventProc);
	
	XM_unlock ();
}

// ���������ر�
void XM_KeyDriverClose (void)
{
	XM_lock ();
	// ���ں�ע�������¼�֪ͨ�ص�����
	//XM_RegisterKeyEventProc (NULL);

	keyInit = 0;

	KeyBPos = KeyEPos = 0;
	
	XM_unlock ();
}


//=========================
// ��ѯ�Ƿ����δ���İ����¼�
XMBOOL XM_KeyDriverPoll(void)
{
	XMBOOL ret = FALSE;

	XM_lock();	// �����������
	if( KeyEPos != KeyBPos )	/* Key Event buffer empty */
		ret = TRUE;
	XM_unlock();

	return ret;
}

//=====================

// ��鲢Ͷ��Ӳ���¼�, ���ⲿӲ����������
// ����¼�δ��Ӳ����Ϣ������, �����¼����뵽��β.
// �����¼�����Ӳ����Ϣ������, �����κδ���, ����
// ����ֵ
//   1        ��ʾ����Ϣ��Ͷ�ݵ�Ӳ������
//   0        ��ʾ����Ϣδ��Ͷ�ݵ�Ӳ������, ��ϢͶ��ʧ��
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
		
		// ����, ���Ӳ���¼��Ƿ��Ѵ���
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
		
		// �¼�δ����
		ret = XM_KeyEventProc (key, mod);		
		
	} while (0);
	XM_unlock ();
	
	return ret;
}
